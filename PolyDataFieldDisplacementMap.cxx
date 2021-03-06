/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "PolyDataFieldDisplacementMap.h"

#include "SQMacros.h"
#include "IdBlock.h"
#include "WorkQueue.h"
#include "FieldLine.h"
#include "TerminationCondition.h"
#include "vtkSQCellGenerator.h"
#include "vtkDataSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIdTypeArray.h"
#include "vtkCellType.h"

//-----------------------------------------------------------------------------
PolyDataFieldDisplacementMap::~PolyDataFieldDisplacementMap()
{
  this->ClearSource();
  this->ClearOut();
}

//-----------------------------------------------------------------------------
void PolyDataFieldDisplacementMap::ClearSource()
{
  if (this->SourceGen){ this->SourceGen->Delete(); }
  if (this->SourcePts){ this->SourcePts->Delete(); }
  if (this->SourceCells){ this->SourceCells->Delete(); }
  this->SourceGen=0;
  this->SourcePts=0;
  this->SourceCells=0;
  this->IdMap.clear();
  this->CellType=0;
}

//-----------------------------------------------------------------------------
void PolyDataFieldDisplacementMap::ClearOut()
{
  if (this->OutPts){ this->OutPts->Delete(); }
  if (this->OutCells){ this->OutCells->Delete(); }
  this->OutPts=0;
  this->OutCells=0;
  this->IdMap.clear();
  // this->CellType=0;
}

//-----------------------------------------------------------------------------
void PolyDataFieldDisplacementMap::SetSource(vtkSQCellGenerator *sourceGen)
{
  if (this->SourceGen==sourceGen){ return; }
  if (this->SourceGen){ this->SourceGen->Delete(); }
  this->SourceGen=sourceGen;
  this->CellType=0;
  if (this->SourceGen)
    {
    this->SourceGen->Register(0);
    this->CellType=this->SourceGen->GetCellType(0);
    }
}

//-----------------------------------------------------------------------------
void PolyDataFieldDisplacementMap::SetSource(vtkDataSet *s)
{
  this->ClearSource();

  vtkPolyData *source=dynamic_cast<vtkPolyData*>(s);
  if (source==0)
    {
    sqErrorMacro(cerr,
      "Error: Source must be polydata. " << s->GetClassName());
    return;
    }

  this->SourcePts=dynamic_cast<vtkFloatArray*>(source->GetPoints()->GetData());
  if (this->SourcePts==0)
    {
    sqErrorMacro(cerr,"Error: Points are not float precision.");
    return;
    }
  this->SourcePts->Register(0);

  if (source->GetNumberOfPolys())
    {
    this->SourceCells=source->GetPolys();
    this->CellType=VTK_POLYGON;
    }
  else
  if (source->GetNumberOfVerts())
    {
    this->SourceCells=source->GetVerts();
    this->CellType=VTK_VERTEX;
    }
  else
    {
    sqErrorMacro(cerr,
        "Error: Polydata doesn't have any supported cells.");
    return;
    }
  this->SourceCells->Register(0);
}

//-----------------------------------------------------------------------------
void PolyDataFieldDisplacementMap::SetOutput(vtkDataSet *o)
{
  this->FieldDisplacementMapData::SetOutput(o);

  this->ClearOut();

  vtkPolyData *out=dynamic_cast<vtkPolyData*>(o);
  if (out==0)
    {
    sqErrorMacro(cerr,"Error: Out must be polydata. " << o->GetClassName());
    return;
    }

  vtkPoints *opts=vtkPoints::New();
  out->SetPoints(opts);
  opts->Delete();
  this->OutPts=dynamic_cast<vtkFloatArray*>(opts->GetData());
  this->OutPts->Register(0);

  this->OutCells=vtkCellArray::New();
  switch (this->CellType)
    {
    case VTK_POLYGON:
      out->SetPolys(this->OutCells);
      break;

    case VTK_VERTEX:
      out->SetVerts(this->OutCells);
      break;

    default:
      sqErrorMacro(cerr,"Error: Unsuported cell type.");
      return;
    }
}

//-----------------------------------------------------------------------------
int PolyDataFieldDisplacementMap::InsertCells(IdBlock *SourceIds)
{
  if (this->SourceGen)
    {
    return this->InsertCellsFromGenerator(SourceIds);
    }
  return this->InsertCellsFromDataset(SourceIds);
}

//-----------------------------------------------------------------------------
int PolyDataFieldDisplacementMap::InsertCellsFromGenerator(IdBlock *SourceIds)
{
  vtkIdType startCellId=SourceIds->first();
  vtkIdType nCellsLocal=SourceIds->size();

  // update the cell count before we forget (does not allocate).
  this->OutCells->SetNumberOfCells(OutCells->GetNumberOfCells()+nCellsLocal);

  vtkIdTypeArray *outCells=this->OutCells->GetData();
  vtkIdType insertLoc=outCells->GetNumberOfTuples();

  vtkIdType nOutPts=this->OutPts->GetNumberOfTuples();
  vtkIdType polyId=startCellId;

  vector<vtkIdType> sourcePtIds;
  vector<float> sourcePts;

  // For each cell asigned to us we'll get its center (this is the seed point)
  // and build corresponding cell in the output, The output only will have
  // the cells assigned to this process.
  for (vtkIdType i=0; i<nCellsLocal; ++i)
    {
    // Get the cell that belong to us.
    vtkIdType nSourcePtIds=this->SourceGen->GetNumberOfCellPoints(polyId);
    sourcePtIds.resize(nSourcePtIds);
    sourcePts.resize(3*nSourcePtIds);
    this->SourceGen->GetCellPointIndexes(polyId,&sourcePtIds[0]);
    this->SourceGen->GetCellPoints(polyId,&sourcePts[0]);

    // Get location to write new cell.
    vtkIdType *pOutCells=outCells->WritePointer(insertLoc,nSourcePtIds+1);
    // update next cell write location.
    insertLoc+=nSourcePtIds+1;
    // number of points in this cell
    *pOutCells=nSourcePtIds;
    ++pOutCells;

    // Get location to write new point. assumes we need to copy all
    // but this is wrong as there will be many duplicates. ignored.
    float *pOutPts=this->OutPts->WritePointer(3*nOutPts,3*nSourcePtIds);

    // transfer from input to output (only what we own)
    for (vtkIdType j=0; j<nSourcePtIds; ++j,++pOutCells)
      {
      vtkIdType idx=3*j;
      // do we already have this point?
      MapElement elem(sourcePtIds[j],nOutPts);
      MapInsert ret=this->IdMap.insert(elem);
      if (ret.second==true)
        {
        // this point hasn't previsouly been coppied
        // copy the point.
        pOutPts[0]=sourcePts[idx  ];
        pOutPts[1]=sourcePts[idx+1];
        pOutPts[2]=sourcePts[idx+2];

        // insert the new point id.
        *pOutCells=nOutPts;

        // compute a field line from this point
        FieldLine *line=new FieldLine(pOutPts,nOutPts);
        line->AllocateTrace();
        this->Lines.push_back(line);

        nOutPts+=1;
        pOutPts+=3;
        }
      else
        {
        // this point has been coppied, do not add a duplicate.
        // insert the other point id.
        *pOutCells=(*ret.first).second;
        }
      }

    ++polyId;
    }
  // correct the length of the point array, above we assumed 
  // that all points from each cell needed to be inserted
  // and allocated that much space.
  this->OutPts->Resize(nOutPts);

  return this->Lines.size();
}

//-----------------------------------------------------------------------------
int PolyDataFieldDisplacementMap::InsertCellsFromDataset(IdBlock *SourceIds)
{
  vtkIdType startCellId=SourceIds->first();
  vtkIdType nCellsLocal=SourceIds->size();

  // Cells are sequentially acccessed (not random) so explicitly skip all cells
  // we aren't interested in.
  this->SourceCells->InitTraversal();
  for (vtkIdType i=0; i<startCellId; ++i)
    {
    vtkIdType n;
    vtkIdType *ptIds;
    this->SourceCells->GetNextCell(n,ptIds);
    }

  // update the cell count before we forget (does not allocate).
  this->OutCells->SetNumberOfCells(OutCells->GetNumberOfCells()+nCellsLocal);

  float *pSourcePts=this->SourcePts->GetPointer(0);

  vtkIdTypeArray *outCells=this->OutCells->GetData();
  vtkIdType insertLoc=outCells->GetNumberOfTuples();

  vtkIdType nOutPts=this->OutPts->GetNumberOfTuples();

  // For each cell asigned to us we'll get its center (this is the seed point)
  // and build corresponding cell in the output, The output only will have
  // the cells assigned to this process.
  for (vtkIdType i=0; i<nCellsLocal; ++i)
    {
    // Get the cell that belong to us.
    vtkIdType nPtIds=0;
    vtkIdType *ptIds=0;
    this->SourceCells->GetNextCell(nPtIds,ptIds);

    // Get location to write new cell.
    vtkIdType *pOutCells=outCells->WritePointer(insertLoc,nPtIds+1);
    // update next cell write location.
    insertLoc+=nPtIds+1;
    // number of points in this cell
    *pOutCells=nPtIds;
    ++pOutCells;

    // Get location to write new point. assumes we need to copy all
    // but this is wrong as there will be many duplicates. ignored.
    float *pOutPts=this->OutPts->WritePointer(3*nOutPts,3*nPtIds);

    // transfer from input to output (only what we own)
    for (vtkIdType j=0; j<nPtIds; ++j,++pOutCells)
      {
      vtkIdType idx=3*ptIds[j];
      // do we already have this point?
      MapElement elem(ptIds[j],nOutPts);
      MapInsert ret=this->IdMap.insert(elem);
      if (ret.second==true)
        {
        // this point hasn't previsouly been coppied
        // copy the point.
        pOutPts[0]=pSourcePts[idx  ];
        pOutPts[1]=pSourcePts[idx+1];
        pOutPts[2]=pSourcePts[idx+2];

        // insert the new point id.
        *pOutCells=nOutPts;

        // compute a field line form this point
        FieldLine *line=new FieldLine(pOutPts,nOutPts);
        line->AllocateTrace();
        this->Lines.push_back(line);

        nOutPts+=1;
        pOutPts+=3;
        }
      else
        {
        // this point has been coppied, do not add a duplicate.
        // insert the other point id.
        *pOutCells=(*ret.first).second;
        }
      }
    }
  // correct the length of the point array, above we assumed
  // that all points from each cell needed to be inserted
  // and allocated that much space.
  this->OutPts->Resize(nOutPts);

  return this->Lines.size();
}
