/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQRandomCells.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQRandomCells.h"
 
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMultiProcessController.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkType.h"
#include "vtkCellType.h"

#include "SQMacros.h"
#include "CellIdBlock.h"
#include "Numerics.hxx"
#include "Tuple.hxx"

#include <set>
using std::set;
#include <map>
using std::map;
using std::pair;

#include <cstdlib>
#include <ctime>

#include <mpi.h>

typedef pair<set<int>::iterator,bool> SetInsert;
typedef pair<map<vtkIdType,vtkIdType>::iterator,bool> MapInsert;
typedef pair<vtkIdType,vtkIdType> MapElement;

// #define vtkSQRandomCellsDEBUG

vtkCxxRevisionMacro(vtkSQRandomCells, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkSQRandomCells);


//*****************************************************************************
int findProcByCellId(int cellId, CellIdBlock *bins, int s, int e)
{
  // binary search for rank who owns the given cell id.

  if (s>e || e<s)
    {
    sqErrorMacro(cerr,"Index error s=" << s << " e=" << e << ".");
    return -1;
    }

  int m=(s+e)/2;

  if (bins[m].contains(cellId))
    {
    cerr << "proc=" << m << " owns " << cellId << "." << endl;
    return m;
    }
  else
  if ((cellId<bins[m].first())&&(s!=e))
    {
    return findProcByCellId(cellId,bins,s,m-1);
    }
  else
  if ((cellId>(bins[m].last()-1))&&(s!=e))
    {
    return findProcByCellId(cellId,bins,m+1,e);
    }

  // not found
  sqErrorMacro(cerr,"Error: CellId=" << cellId << " was not found.");
  return -1;
}

//*****************************************************************************
int copyCell(
      vtkIdType cellId,
      vtkCellArray *sourceCells,
      vtkFloatArray *sourcePts,
      vtkCellArray *outCells,
      vtkFloatArray *outPts,
      map<vtkIdType,vtkIdType> &idMap)
{

  // Cells are sequentially acccessed (not random) so explicitly skip all cells
  // we aren't interested in.
  sourceCells->InitTraversal();
  for (vtkIdType i=0; i<cellId; ++i)
    {
    vtkIdType n;
    vtkIdType *ptIds;
    sourceCells->GetNextCell(n,ptIds);
    }

  // update the cell count before we forget (does not allocate).
  outCells->SetNumberOfCells(outCells->GetNumberOfCells()+1);

  float *pSourcePts=sourcePts->GetPointer(0);

  vtkIdType insertLoc=outCells->GetData()->GetNumberOfTuples();

  vtkIdType nOutCellPts=outPts->GetNumberOfTuples();

  // Get the cell that belong to us.
  vtkIdType nPtIds=0;
  vtkIdType *ptIds=0;
  sourceCells->GetNextCell(nPtIds,ptIds);

  // Get location to write new cell.
  vtkIdType *pOutCells
    = outCells->GetData()->WritePointer(insertLoc,nPtIds+1);

  // update next cell write location.
  insertLoc+=nPtIds+1;
  // number of points in this cell
  *pOutCells=nPtIds;
  ++pOutCells;

  // Get location to write new point. assumes we need to copy all
  // but this is wrong as there will be many duplicates. ignored.
  float *pOutPts=outPts->WritePointer(3*nOutCellPts,3*nPtIds);

  // transfer from input to output (only what we own)
  for (vtkIdType j=0; j<nPtIds; ++j,++pOutCells)
    {
    vtkIdType idx=3*ptIds[j];
    // do we already have this point?
    MapElement elem(ptIds[j],nOutCellPts);
    MapInsert ret=idMap.insert(elem);
    if (ret.second==true)
      {
      // this point hasn't previsouly been coppied
      // copy the point.
      pOutPts[0]=pSourcePts[idx  ];
      pOutPts[1]=pSourcePts[idx+1];
      pOutPts[2]=pSourcePts[idx+2];
      pOutPts+=3;

      // insert the new point id.
      *pOutCells=nOutCellPts;
      ++nOutCellPts;
      }
    else
      {
      // this point has been coppied, do not add a duplicate.
      // insert the other point id.
      *pOutCells=(*ret.first).second;
      }
    }

  // correct the length of the point array, above we assumed 
  // that all points from each cell needed to be inserted
  // and allocated that much space.
  outPts->SetNumberOfTuples(nOutCellPts);

  return 1;
}

//----------------------------------------------------------------------------
vtkSQRandomCells::vtkSQRandomCells()
{
  #ifdef vtkSQRandomCellsDEBUG
  cerr << "===============================vtkSQRandomCells::vtkSQRandomCells" << endl;
  #endif

  this->SampleSize=0;

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkSQRandomCells::~vtkSQRandomCells()
{
  #ifdef vtkSQRandomCellsDEBUG
  cerr << "===============================vtkSQRandomCells::~vtkSQRandomCells" << endl;
  #endif
}

//----------------------------------------------------------------------------
int vtkSQRandomCells::RequestInformation(
    vtkInformation */*req*/,
    vtkInformationVector **/*inInfos*/,
    vtkInformationVector *outInfos)
{
  #ifdef vtkSQRandomCellsDEBUG
  cerr << "===============================vtkSQRandomCells::RequestInformation" << endl;
  #endif

  // tell the excutive that we are handling our own paralelization.
  vtkInformation *outInfo=outInfos->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);

  // TODO extract bounds and set if the input data set is present.

  return 1;
}

//----------------------------------------------------------------------------
int vtkSQRandomCells::RequestData(
    vtkInformation */*req*/,
    vtkInformationVector **inInfos,
    vtkInformationVector *outInfos)
{
  #ifdef vtkSQRandomCellsDEBUG
  cerr << "===============================vtkSQRandomCells::RequestData" << endl;
  #endif

  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);

  vtkPolyData *source
    = dynamic_cast<vtkPolyData*>(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (source==NULL)
    {
    vtkErrorMacro("Empty input.");
    return 1;
    }

  vtkInformation *outInfo=outInfos->GetInformationObject(0);

  vtkPolyData *output
    = dynamic_cast<vtkPolyData*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (output==NULL)
    {
    vtkErrorMacro("Empty output.");
    return 1;
    }

  // sanity - user set invalid number of cells.
  if (this->SampleSize<1)
    {
    vtkErrorMacro("Number of cells must be greater than 0.");
    output->Initialize();
    return 1;
    }

  // paralelize by piece information.
  int pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  int worldSize=1;
  MPI_Comm_size(MPI_COMM_WORLD,&worldSize);

  int worldRank=0;
  MPI_Comm_rank(MPI_COMM_WORLD,&worldRank);

  // sanity - the requst cannot be fullfilled (not necessarilly an error)
  if (pieceNo>=nPieces)
    {
    output->Initialize();
    // return 1;
    // even though no work for this rank, he will participate in
    // collective communication.
    }

  // count of the cells we own.
  int nLocalCells=source->GetNumberOfCells();

  // number and local cell ids of cells which we own which
  // are to be passed through to the output.
  int nCellsToPass=0;
  vector<int> cellsToPass;

  const int masterRank=(worldSize==1?0:1);

  // divi up the work, all process must participate.
  if (worldRank==masterRank)
    {
    // get counts of all cells.
    int nRemoteCells[worldSize];
    MPI_Gather(
        &nLocalCells,
        1,
        MPI_INT,
        &nRemoteCells[0],
        1,
        MPI_INT,
        masterRank,
        MPI_COMM_WORLD);

    // construct the cell id block owned by each process.
    int nCellsTotal=0;
    CellIdBlock remoteCellIds[worldSize];
    for (int i=0; i<worldSize; ++i)
      {
      remoteCellIds[i].first()=nCellsTotal;
      remoteCellIds[i].size()=nRemoteCells[i];
      nCellsTotal+=nRemoteCells[i];
      }

    // select cells to pass through. assigned to the process who
    // owns them.
    int nAssigned[worldSize];
    for (int i=0; i<worldSize; ++i)
      {
      nAssigned[i]=0;
      }
    vector<int> assignments[worldSize];
    ///srand(time(0));
    set<int> usedCellIds;
    SetInsert ok;
    for (int i=0; i<this->SampleSize; ++i)
      {
      int cellId=0;
      do
        {
        cellId=(int)((double)nCellsTotal*(double)rand()/(double)RAND_MAX);
        ok=usedCellIds.insert(cellId);
        }
      while (!ok.second);
      int rank=findProcByCellId(cellId,remoteCellIds,0,worldSize-1);
      cellId-=remoteCellIds[rank].first();
      assignments[rank].push_back(cellId);
      ++nAssigned[rank];
      }

    // distribute the assignments
    for (int i=0; i<worldSize; ++i)
      {
      if (i==masterRank)
        {
        nCellsToPass=nAssigned[i];
        cellsToPass=assignments[i];
        continue;
        }
      MPI_Send(&nAssigned[i],1,MPI_INT,i,0,MPI_COMM_WORLD);
      MPI_Send(&((assignments[i])[0]),nAssigned[i],MPI_INT,i,1,MPI_COMM_WORLD);
      }
    }
  else
    {
    // send a count of the cells we own.
    MPI_Gather(
        &nLocalCells,
        1,
        MPI_INT,
        0,
        0,
        MPI_INT,
        masterRank,
        MPI_COMM_WORLD);

    // obtain our assignments
    MPI_Status stat;
    MPI_Recv(
          &nCellsToPass,
          1,
          MPI_INT,
          masterRank,
          0,
          MPI_COMM_WORLD,
          &stat);

    cellsToPass.resize(nCellsToPass);
    MPI_Recv(
          &cellsToPass[0],
          nCellsToPass,
          MPI_INT,
          masterRank,
          1,
          MPI_COMM_WORLD,
          &stat);
    }

  // OK to stop here, no further collective communications.
  if (nCellsToPass<1)
    {
    return 1;
    }

  vtkCellArray *sourceCells=source->GetPolys();
  if (sourceCells==NULL)
    {
    sqErrorMacro(cerr,"Source does not conatin polygonal cells.");
    return 0;
    }

  vtkFloatArray *sourcePts
    = dynamic_cast<vtkFloatArray*>(source->GetPoints()->GetData());
  if (sourcePts==NULL)
    {
    sqErrorMacro(cerr,"Source does not contain single precision points.");
    return 0;
    }

  vtkCellArray *outCells=vtkCellArray::New();
  output->SetPolys(outCells);
  outCells->Delete();

  vtkPoints *points=vtkPoints::New();
  output->SetPoints(points);
  points->Delete();
  vtkFloatArray *outPts
    = dynamic_cast<vtkFloatArray*>(points->GetData());

  map<vtkIdType,vtkIdType> usedPts;
  for (int i=0; i<nCellsToPass; ++i)
    {
    int ok=copyCell(cellsToPass[i],sourceCells,sourcePts,outCells,outPts,usedPts);
    if (!ok)
      {
      vtkErrorMacro("faild to copy cell " << cellsToPass[i] << ".");
      return 1;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkSQRandomCells::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef vtkSQRandomCellsDEBUG
  cerr << "===============================vtkSQRandomCells::PrintSelf" << endl;
  #endif

  this->Superclass::PrintSelf(os,indent);

  // TODO
}
