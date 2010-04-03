/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "PolyDataFieldTopologyMap.h"

#include "WorkQueue.h"
#include "FieldLine.h"
#include "TerminationCondition.h"
#include "vtkDataSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIdTypeArray.h"

//-----------------------------------------------------------------------------
PolyDataFieldTopologyMap::~PolyDataFieldTopologyMap()
{
  this->ClearSource();
  this->ClearOut();
}

//-----------------------------------------------------------------------------
void PolyDataFieldTopologyMap::ClearSource()
{
  if (this->SourcePts){ this->SourcePts->Delete(); }
  if (this->SourceCells){ this->SourceCells->Delete(); }
  this->SourcePts=0;
  this->SourceCells=0;
  this->IdMap.clear();
  this->CellType=NONE;
}

//-----------------------------------------------------------------------------
void PolyDataFieldTopologyMap::ClearOut()
{
  if (this->OutPts){ this->OutPts->Delete(); }
  if (this->OutCells){ this->OutCells->Delete(); }
  this->OutPts=0;
  this->OutCells=0;
  this->IdMap.clear();
  // this->CellType=NONE;
}

//-----------------------------------------------------------------------------
void PolyDataFieldTopologyMap::SetSource(vtkDataSet *s)
{
  this->ClearSource();

  vtkPolyData *source=dynamic_cast<vtkPolyData*>(s);
  if (source==0)
    {
    cerr << "Error: Source must be polydata. " << s->GetClassName() << endl;
    return;
    }

  this->SourcePts=dynamic_cast<vtkFloatArray*>(source->GetPoints()->GetData());
  if (this->SourcePts==0)
    {
    cerr << "Error: Points are not float precision." << endl;
    return;
    }
  this->SourcePts->Register(0);

  if (source->GetNumberOfPolys())
    {
    this->SourceCells=source->GetPolys();
    this->CellType=POLY;
    }
  else
  if (source->GetNumberOfVerts())
    {
    this->SourceCells=source->GetVerts();
    this->CellType=VERT;
    }
  else
    {
    cerr << "Error: Polydata doesn't have any supported cells." << endl;
    return;
    }
  this->SourceCells->Register(0);
}

//-----------------------------------------------------------------------------
void PolyDataFieldTopologyMap::SetOutput(vtkDataSet *o)
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
  switch (this->CellType)
    {
    case POLY:
      out->SetPolys(this->OutCells);
      break;
    case VERT:
      out->SetVerts(this->OutCells);
      break;
    default:
      cerr << "Error: Unsuported cell type." << endl;
      return;
    }
}

//-----------------------------------------------------------------------------
int PolyDataFieldTopologyMap::InsertCells(CellIdBlock *SourceIds)
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

  vtkIdTypeArray *OutCellCells=this->OutCells->GetData();
  vtkIdType insertLoc=OutCellCells->GetNumberOfTuples();

  vtkIdType nOutCellPts=this->OutPts->GetNumberOfTuples();
  vtkIdType polyId=startCellId;

  int lId=this->Lines.size();
  this->Lines.resize(lId+nCellsLocal,0);

  // For each cell asigned to us we'll get its center (this is the seed point)
  // and build corresponding cell in the output, The output only will have
  // the cells assigned to this process.
  for (vtkIdType i=0; i<nCellsLocal; ++i)
    {
    // Get the cell that belong to us.
    vtkIdType nPtIds;
    vtkIdType *ptIds;
    this->SourceCells->GetNextCell(nPtIds,ptIds);

    // Get location to write new cell.
    vtkIdType *pOutCellCells=OutCellCells->WritePointer(insertLoc,nPtIds+1);
    // update next cell write location.
    insertLoc+=nPtIds+1;
    // number of points in this cell
    *pOutCellCells=nPtIds;
    ++pOutCellCells;

    // Get location to write new point. assumes we need to copy all
    // but this is wrong as there will be many duplicates. ignored.
    float *pOutCellPts=this->OutPts->WritePointer(3*nOutCellPts,3*nPtIds);
    // the  point we will use the center of the cell
    double seed[3]={0.0};
    // transfer from input to output (only what we own)
    for (vtkIdType j=0; j<nPtIds; ++j,++pOutCellCells)
      {
      vtkIdType idx=3*ptIds[j];
      // do we already have this point?
      MapElement elem(ptIds[j],nOutCellPts);
      MapInsert ret=this->IdMap.insert(elem);
      if (ret.second==true)
        {
        // this point hasn't previsouly been coppied
        // copy the point.
        pOutCellPts[0]=pSourcePts[idx  ];
        pOutCellPts[1]=pSourcePts[idx+1];
        pOutCellPts[2]=pSourcePts[idx+2];
        pOutCellPts+=3;

        // insert the new point id.
        *pOutCellCells=nOutCellPts;
        ++nOutCellPts;
        }
      else
        {
        // this point has been coppied, do not add a duplicate.
        // insert the other point id.
        *pOutCellCells=(*ret.first).second;
        }
      // compute contribution to cell center.
      seed[0]+=pSourcePts[idx  ];
      seed[1]+=pSourcePts[idx+1];
      seed[2]+=pSourcePts[idx+2];
      }
    // finsih the seed point computation (at cell center).
    seed[0]/=nPtIds;
    seed[1]/=nPtIds;
    seed[2]/=nPtIds;

    this->Lines[lId]=new FieldLine(seed,polyId);
    ++polyId;
    ++lId;
    }
  // correct the length of the point array, above we assumed 
  // that all points from each cell needed to be inserted
  // and allocated that much space.
  this->OutPts->SetNumberOfTuples(nOutCellPts);

  return nCellsLocal;
}
