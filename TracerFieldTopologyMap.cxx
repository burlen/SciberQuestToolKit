/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "TracerFieldTopologyMap.h"

#include "CellIdBlock.h"
#include "FieldLine.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIdTypeArray.h"

//-----------------------------------------------------------------------------
TracerFieldTopologyMap::~TracerFieldTopologyMap()
{
  this->ClearSource();
  this->ClearOut();
}

//-----------------------------------------------------------------------------
void TracerFieldTopologyMap::ClearSource()
{
  if (this->SourcePts){ this->SourcePts->Delete(); }
  if (this->SourceCells){ this->SourceCells->Delete(); }
  this->SourcePts=0;
  this->SourceCells=0;
  this->IdMap.Clear();
  this->CellType=NONE;
}

//-----------------------------------------------------------------------------
void TracerFieldTopologyMap::ClearOut()
{
  if (this->OutPts){ this->OutPts->Delete(); }
  if (this->OutCells){ this->OutCells->Delete(); }
  this->OutPts=0;
  this->OutCells=0;
  this->IdMap.Clear();
  this->CellType=NONE;
}

//-----------------------------------------------------------------------------
void TracerFieldTopologyMap::SetSource(vtkDataSet *s)
{
  this->ClearSource();

  // unstructured (the easier)
  vtkUnstructuredGrid *sourceug=dynamic_cast<vtkUnstructuredGrid>(s);
  if (sourceug)
    {
    this->SourcePts=dynamic_cast<vtkFloatArray*>(sourceug->GetPoints()->GetData());
    if (this->SourcePoints==0)
      {
      cerr << "Error: Points are not float precision." << endl;
      return;
      }
    this->SourcePts->Register(0);

    this->SourceCells=sourceug->GetCells();
    this->SourceCells->Register(0);

    return;
    }

  // polydata
  vtkPolyDataGrid *sourcepd=dynamic_cast<vtkPolyDataGrid>(s);
  if (sourcepd)
    {
    this->SourcePts=dynamic_cast<vtkFloatArray*>(sourcepd->GetPoints()->GetData());
    if (this->SourcePoints==0)
      {
      cerr << "Error: Points are not float precision." << endl;
      return;
      }
    this->SourcePts->Register(0);

    if (sourcepd->GetNumberOfPolys())
      {
      this->SourceCells=sourcepd->GetPolys();
      }
    else
    if (sourcepd->GetNumberOfVerts())
      {
      this->SourceCells=sourcepd->GetVerts();
      }
    else
      {
      cerr << "Error: Polydata doesn't have any supported cells." << endl;
      return;
      }
    this->SourceCells->Register(0);
    return;
    }


  cerr << "Error: Unsupported input data type." << endl;
}

//-----------------------------------------------------------------------------
void TracerFieldTopologyMap::SetOutput(vtkDataSet *o)
{
  this->FieldTopologyMap::SetOutput(o);

  this->ClearOut();

  vtkPolyData *out=dynamic_cast<vtkPolyData*>(o);
  if (out==0)
    {
    cerr << "Error: Out must be polydata. " << o->GetClassName() << endl;
    return;
    }

  vtkPoints *opts=vtkPoints::New();
  out->SetPoints(opts);
  opts->Delete();
  this->OutPts=dynamic_cast<vtkFloatArray*>(opts->GetData());
  this->OutPts->Register(0);

  this->OutCells=vtkCellArray::New();
  out->SetLines(this->OutCells);
}

//-----------------------------------------------------------------------------
int TracerFieldTopologyMap::InsertCells(
      CellIdBlock *SourceIds,
      vector<FieldLine *> &lines)
{
  vtkIdType startId=SourceIds->first();
  vtkIdType endId=SourceIds->last();
  vtkIdType nCellsLocal=SourceIds->size();

  int lId=lines.size();
  lines.resize(lId+nCellsLocal,0);

  float *pSourcePts=this->SourcePts->GetPointer(0);

  // Cells are sequentially acccessed (not random) so explicitly skip all cells
  // we aren't interested in.
  this->SourceCells->InitTraversal();
  for (vtkIdType i=0; i<startCellId; ++i)
    {
    vtkIdType n;
    vtkIdType *ptIds;
    SourceCells->GetNextCell(n,ptIds);
    }

  // For each cell asigned to us we'll get its center (this is the seed point)
  // and build corresponding cell in the output, The output only will have
  // the cells assigned to this process.
  vtkIdList *ptIds=vtkIdList::New();
  for (vtkIdType cId=startId; cId<endId; ++cId)
    {
    // get the cell that belong to us.
    vtkIdType nPtIds;
    vtkIdType *ptIds;
    SourceCells->GetNextCell(nPtIds,ptIds);

    // the seed point we will use the center of the cell
    double seed[3]={0.0};
    // transfer from input to output (only what we own)
    for (vtkIdType pId=0; pId<nPtIds; ++pId)
      {
      vtkIdType idx=3*ptIds[pId];
      // compute contribution to cell center.
      seed[0]+=pSourcePts[idx  ];
      seed[1]+=pSourcePts[idx+1];
      seed[2]+=pSourcePts[idx+2];
      }
    // finsih the seed point computation (at cell center).
    seed[0]/=nPtIds;
    seed[1]/=nPtIds;
    seed[2]/=nPtIds;

    lines[lId]=new FieldLine(seed,cId);
    ++lId;
    }
  ptIds->Delete();

  return nCellsLocal;
}

//-----------------------------------------------------------------------------
int TracerFieldTopologyMap::SyncGeometry()
{

  size_t nLines=this->Lines.size();

  vtkIdType nPtsTotal=0;
  for (size_t i=0; i<nLines; ++i)
    {
    nPtsTotal+=this->Lines[i]->GetNumberOfPoints();
    }

  vtkIdType nOutPts=this->OutPts->GetNumberOfTuples();
  float *pOutPts=this->OutPts->WritePointer(3*nOutPts,3*nPtsTotal);

  vtkIdType nLineCells=this->OutCells->GetNumberOfCells();
  vtkIdTypeArray *lineCells=this->OutCells->GetData();
  vtkIdType *pLineCells=lineCells->WritePointer(nLineCells,nPtsTotal+nLines);

  vtkIdType ptId=nLinePts;

  for (size_t i=0; i<nLines; ++i)
    {
    // copy the points
    vtkIdType nLinePts=this->Lines[i]->CopyPointsTo(pLinePts);
    pLinePts+=3*nLinePts;

    // build the cell
    *pLineCells=nLinePts;
    ++pLineCells;
    for (vtkIdType q=0; q<nLinePts; ++q)
      {
      *pLineCells=ptId;
      ++pLineCells;
      ++ptId;
      }

    this->Lines[i]->DeleteTrace();
    }

  return 1;
}

