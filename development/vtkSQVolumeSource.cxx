/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQVolumeSource.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQVolumeSource.h"
 
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
#include "vtkUnstructuredGrid.h"
#include "vtkType.h"
#include "vtkCellType.h"
#include "Numerics.hxx"
#include "Tuple.hxx"

#include <map>
using std::map;
using std::pair;

typedef pair<map<vtkIdType,vtkIdType>::iterator,bool> MapInsert;
typedef pair<vtkIdType,vtkIdType> MapElement;

// #define vtkSQVolumeSourceDEBUG

vtkCxxRevisionMacro(vtkSQVolumeSource, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkSQVolumeSource);

//----------------------------------------------------------------------------
vtkSQVolumeSource::vtkSQVolumeSource()
{
  #ifdef vtkSQVolumeSourceDEBUG
  cerr << "===============================vtkSQVolumeSource::vtkSQVolumeSource" << endl;
  #endif

  this->NCells[0]=
  this->NCells[1]=
  this->NCells[2]=1;

  this->Origin[0]=
  this->Origin[1]=
  this->Origin[2]=0.0;

  this->Point1[0]=1.0;
  this->Point1[1]=0.0;
  this->Point1[2]=0.0;

  this->Point2[0]=0.0;
  this->Point2[1]=1.0;
  this->Point2[2]=0.0;

  this->Point3[0]=0.0;
  this->Point3[1]=0.0;
  this->Point3[2]=1.0;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkSQVolumeSource::~vtkSQVolumeSource()
{
  #ifdef vtkSQVolumeSourceDEBUG
  cerr << "===============================vtkSQVolumeSource::~vtkSQVolumeSource" << endl;
  #endif
}

// //----------------------------------------------------------------------------
// int vtkSQVolumeSource::FillInputPortInformation(
//       int /*port*/,
//       vtkInformation *info)
// {
//   #ifdef vtkSQVolumeSourceDEBUG
//   cerr << "===============================vtkSQVolumeSource::FillInputPortInformation" << endl;
//   #endif
// 
//   // The input is optional,if present it will be used 
//   // for bounds.
//   info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkDataSet");
//   info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(),1);
// 
//   return 1;
// }

//----------------------------------------------------------------------------
int vtkSQVolumeSource::RequestInformation(
    vtkInformation */*req*/,
    vtkInformationVector **/*inInfos*/,
    vtkInformationVector *outInfos)
{
  #ifdef vtkSQVolumeSourceDEBUG
  cerr << "===============================vtkSQVolumeSource::RequestInformation" << endl;
  #endif

  // tell the excutive that we are handling our own paralelization.
  vtkInformation *outInfo=outInfos->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);

  // TODO extract bounds and set if the input data set is present.

  return 1;
}

//----------------------------------------------------------------------------
int vtkSQVolumeSource::RequestData(
    vtkInformation */*req*/,
    vtkInformationVector **inInfos,
    vtkInformationVector *outInfos)
{
  #ifdef vtkSQVolumeSourceDEBUG
  cerr << "===============================vtkSQVolumeSource::RequestData" << endl;
  #endif


  vtkInformation *outInfo=outInfos->GetInformationObject(0);

  vtkUnstructuredGrid *output
    = dynamic_cast<vtkUnstructuredGrid*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (output==NULL)
    {
    vtkErrorMacro("Empty output.");
    return 1;
    }

  // paralelize by piece information.
  int pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // sanity - the requst cannot be fullfilled
  if (pieceNo>=nPieces)
    {
    output->Initialize();
    return 1;
    }

  // sanity - user set invalid number of cells
  if ( (this->NCells[0]<1)
    || (this->NCells[1]<1)
    || (this->NCells[2]<1) )
    {
    vtkErrorMacro("Number of cells must be greater than 0.");
    return 1;
    }

  int nPoints[3]={
      this->NCells[0]+1,
      this->NCells[1]+1,
      this->NCells[2]+1
      };

  // constants used in flat index
  const int ncx=this->NCells[0];
  const int ncxy=this->NCells[0]*this->NCells[1];
  const int npx=nPoints[0];
  const int npxy=nPoints[0]*nPoints[1];

  // domain decomposition on cells.
  int nCellsTotal=this->NCells[0]*this->NCells[1]*this->NCells[2];
  int nPointsTotal=nPoints[0]*nPoints[1]*nPoints[2];
  int pieceSize=nCellsTotal/nPieces;
  int nLarge=nCellsTotal%nPieces;
  int nCellsLocal=pieceSize+(pieceNo<nLarge?1:0);
  int startId=pieceSize*pieceNo+(pieceNo<nLarge?pieceNo:nLarge);
  int endId=startId+nCellsLocal;

  // progress
  double prog=0.0;
  double progRepLevel=0.1;
  const double progUnit=1.0/nCellsLocal;
  const double progRepUnit=0.1;

  // generate the volume axes.(no domain decomp)
  double *axes[4]={
      this->Origin,
      this->Point1,
      this->Point2,
      this->Point3
      };

  float *aX[3]={0};

  for (int q=0; q<3; ++q)
    {
    aX[q]=new float [3*nPoints[q]];
    linspace(axes[0],axes[q+1],nPoints[q],aX[q]);
    }

  // points
  vtkPoints *pts=vtkPoints::New();
  output->SetPoints(pts);
  pts->Delete();
  vtkFloatArray *X=dynamic_cast<vtkFloatArray*>(pts->GetData());
  float *pX=X->WritePointer(0,24*nCellsLocal);
  vtkIdType lpid=0;

  // cells
  vtkCellArray *cells=vtkCellArray::New();
  vtkIdType *pCells=cells->WritePointer(nCellsLocal,9*nCellsLocal);

  // cell types
  vtkUnsignedCharArray *types=vtkUnsignedCharArray::New();
  types->SetNumberOfTuples(nCellsLocal);
  unsigned char *pTypes=types->GetPointer(0);

  // cell locations
  vtkIdTypeArray *locs=vtkIdTypeArray::New();
  locs->SetNumberOfTuples(nCellsLocal);
  vtkIdType *pLocs=locs->GetPointer(0);
  vtkIdType loc=0;

  // to prevent duplicate insertion of points
  map<vtkIdType,vtkIdType> usedPointIds;
  vtkIdType nPts=0;

  // generate the point set
  for (int cid=startId; cid<endId; ++cid)
    {
    // update progress
    if (prog>=progRepLevel)
      {
      this->UpdateProgress(prog);
      progRepLevel+=progRepUnit;
      }
    prog+=progUnit;


    // new hexahedral cell

    // cell index
    int i,j,k;
    indexToIJK(cid,ncx,ncxy,i,j,k);

    // point indices in VTK order.
    int I[24]={
        i  ,j  ,k  ,
        i+1,j  ,k  ,
        i+1,j+1,k  ,
        i  ,j+1,k  ,
        i  ,j  ,k+1,
        i+1,j  ,k+1,
        i+1,j+1,k+1,
        i  ,j+1,k+1
        };
    int *pI=I;

    // cell size
    *pCells=8;
    ++pCells;

    for (int q=0; q<8; ++q)
      {
      // avoid duplicates by keeping a record of which
      // point ids have been used. The global index is
      // used as a key, the local index is stored.
      vtkIdType gpid=pI[0]+pI[1]*npx+pI[2]*npxy;

      MapElement elem(gpid,lpid);
      MapInsert ins=usedPointIds.insert(elem);
      if (ins.second)
        {
        // point has not been previously inserted. Insert it
        // now.
        double *o=this->Origin;
        float *x1=aX[0]+3*pI[0];
        float *x2=aX[1]+3*pI[1];
        float *x3=aX[2]+3*pI[2];

        pX[0]=x1[0]+x2[0]+x3[0]-2.0*o[0];
        pX[1]=x1[1]+x2[1]+x3[1]-2.0*o[1];
        pX[2]=x1[2]+x2[2]+x3[2]-2.0*o[2];
        pX+=3;

        *pCells=lpid;

        ++lpid;
        }
      else
        {
        // point was previously inserted use its local index
        // stored when it was inserted.
        *pCells=(*ins.first).second;
        }

      ++pCells;

      pI+=3;
      }

    // cell location
    *pLocs=loc;
    ++pLocs;
    loc+=9;

    // cell type
    *pTypes=VTK_HEXAHEDRON;
    ++pTypes;
    }

  for (int q=0; q<2; ++q)
    {
    delete [] aX[q];
    }

  // correct possible over-estimation.
  X->SetNumberOfTuples(lpid);

  // transfer
  output->SetCells(types,locs,cells);

  types->Delete();
  locs->Delete();
  cells->Delete();

  #ifdef vtkSQVolumeSourceDEBUG
  int rank=vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
  cerr
    << "pieceNo = " << pieceNo << endl
    << "nPieces = " << nPieces << endl
    << "rank    = " << rank << endl
    << "nCellsLocal  = " << nCellsLocal << endl
    << "startId = " << startId << endl
    << "endId   = " << endId << endl
    << "NCells=" << Tuple<int>(this->NCells,3) << endl
    << "nPoints=" << Tuple<int>(nPoints,3) << endl;
  #endif

  return 1;
}

//----------------------------------------------------------------------------
void vtkSQVolumeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef vtkSQVolumeSourceDEBUG
  cerr << "===============================vtkSQVolumeSource::PrintSelf" << endl;
  #endif

  this->Superclass::PrintSelf(os,indent);

  // TODO
}
