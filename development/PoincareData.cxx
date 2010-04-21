/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "PoincareData.h"

#include "WorkQueue.h"
#include "FieldLine.h"
#include "TerminationCondition.h"
#include "vtkDataSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIdTypeArray.h"

//-----------------------------------------------------------------------------
PoincareData::~PoincareData()
{
  this->ClearSource();
  this->ClearOut();
}

//-----------------------------------------------------------------------------
void PoincareData::ClearSource()
{
  if (this->SourcePts){ this->SourcePts->Delete(); }
  if (this->SourceCells){ this->SourceCells->Delete(); }
  this->SourcePts=0;
  this->SourceCells=0;
}

//-----------------------------------------------------------------------------
void PoincareData::ClearOut()
{
  if (this->OutPts){ this->OutPts->Delete(); }
  if (this->OutCells){ this->OutCells->Delete(); }
  this->OutPts=0;
  this->OutCells=0;
}

//-----------------------------------------------------------------------------
void PoincareData::SetSource(vtkDataSet *s)
{
  this->ClearSource();

  // some process may not have any work, in that case we still
  // should initialize with empty objects.
  if (s->GetNumberOfCells()==0)
    {
    this->SourceCells=vtkCellArray::New();
    this->SourcePts=vtkFloatArray::New();
    return;
    }

  // unstructured (the easier)
  vtkUnstructuredGrid *sourceug=dynamic_cast<vtkUnstructuredGrid*>(s);
  if (sourceug)
    {
    this->SourcePts=dynamic_cast<vtkFloatArray*>(sourceug->GetPoints()->GetData());
    if (this->SourcePts==0)
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
  vtkPolyData *sourcepd=dynamic_cast<vtkPolyData*>(s);
  if (sourcepd)
    {
    this->SourcePts=dynamic_cast<vtkFloatArray*>(sourcepd->GetPoints()->GetData());
    if (this->SourcePts==0)
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
void PoincareData::SetOutput(vtkDataSet *o)
{
  this->FieldTraceData::SetOutput(o);

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
  out->SetVerts(this->OutCells);
}

//-----------------------------------------------------------------------------
int PoincareData::InsertCells(CellIdBlock *SourceIds)
{
  vtkIdType startId=SourceIds->first();
  vtkIdType endId=SourceIds->last();
  vtkIdType nCellsLocal=SourceIds->size();

  int lId=this->Lines.size();
  this->Lines.resize(lId+nCellsLocal,0);

  float *pSourcePts=this->SourcePts->GetPointer(0);

  // Cells are sequentially acccessed (not random) so explicitly skip all cells
  // we aren't interested in.
  this->SourceCells->InitTraversal();
  for (vtkIdType i=0; i<startId; ++i)
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
    // transfer cell center
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

    this->Lines[lId]=new FieldLine(seed,cId);
    this->Lines[lId]->AllocateTrace();
    ++lId;
    }
  ptIds->Delete();

  return nCellsLocal;
}

//-----------------------------------------------------------------------------
int PoincareData::SyncGeometry()
{

  size_t nLines=this->Lines.size();

  vtkIdType nPtsTotal=0;
  for (size_t i=0; i<nLines; ++i)
    {
    nPtsTotal+=this->Lines[i]->GetNumberOfPoints();
    }

  vtkIdType nMapPts=this->OutPts->GetNumberOfTuples();
  float *pMapPts=this->OutPts->WritePointer(3*nMapPts,3*nPtsTotal);

  vtkIdTypeArray *mapCells=this->OutCells->GetData();
  vtkIdType *pMapCells=mapCells->WritePointer(mapCells->GetNumberOfTuples(),2*nPtsTotal);

  // before we forget
  this->OutCells->SetNumberOfCells(this->OutCells->GetNumberOfCells()+nPtsTotal);

  vtkIdType ptId=nMapPts;

  for (size_t i=0; i<nLines; ++i)
    {
    // copy the points
    vtkIdType nMapPts=this->Lines[i]->CopyEndPoints(pMapPts);
    pMapPts+=3*nMapPts;

    // build the verts (either 1 or 2)
    for (int q=0; q<nMapPts; ++q)
      {
      *pMapCells=1;
      ++pMapCells;
      *pMapCells=ptId;
      ++pMapCells;
      ++ptId;
      }

    // NOTE assume we are done after syncing geom.
    delete this->Lines[i];
    this->Lines[i]=0;
    }

  this->Lines.clear();

  return 1;
}

