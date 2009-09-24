
#include "TerminationCondition.h"

#define SafeDelete(a)\
  if (a)\
    {\
    a->Delete();\
    }

//-----------------------------------------------------------------------------
TerminationCondition::~TerminationCondition()
{
  SafeDelete(this->ProblemDomain);
  SafeDelete(this->WorkingDomain);
  this->ClearSurfaces();
}

//-----------------------------------------------------------------------------
void TerminationConditionz::ClearSurfaces()
{
  size_t nSurfaces=this->Surfaces.size();
  for (int i=0; i<nSurfaces; ++i)
    {
    SafeDelete(this->Surfaces[i]);
    }
  this->Surfaces.clear();
}

//-----------------------------------------------------------------------------
void TerminationCondition::SetProblemDomain(double dom[6])
{
  SafeDelete(this->ProblemDomain);
  this->ProblemDomain=vtkCellLocator::New();

  this->DomainToLocator(this->ProblemDomain,dom);
}

//-----------------------------------------------------------------------------
void TerminationCondition::SetWorkingDomain(double dom[6])
{
  SafeDelete(this->WorkingDomain);
  this->WorkingDomain=vtkCellLocator::New();

  this->DomainToLocator(this->WorkingDomain,dom);
}

//-----------------------------------------------------------------------------
void TerminationCondition::PushSurface(vtkPolyData *pd)
{
  this->Surfaces->push_back(pd);
  pd->Register(0);
}

//-----------------------------------------------------------------------------
void TerminationCondition::DomainToLocator(vtkCellLocator *cellLoc, double dom[6])
{
  vtkPoints *verts=vtkPoints::New();
  verts->SetNumberOfPoints(8);
  verts->SetPoint(0, dom[0], dom[2], dom[4]); // b
  verts->SetPoint(1, dom[1], dom[2], dom[4]);
  verts->SetPoint(2, dom[1], dom[3], dom[4]);
  verts->SetPoint(3, dom[0], dom[3], dom[4]);
  verts->SetPoint(4, dom[0], dom[2], dom[5]); // t
  verts->SetPoint(5, dom[1], dom[2], dom[5]);
  verts->SetPoint(6, dom[1], dom[3], dom[5]);
  verts->SetPoint(7, dom[0], dom[3], dom[5]);

  vtkPolyData *surface=vtkPolyData::New();
  surface->SetPoints(verts);
  verts->Delete();

  vtkIdType cellPts[24]
    ={
    0,1,3,2,  // b
    4,5,7,6,  // t
    0,4,7,3,  // f
    1,5,2,6,  // b
    0,4,5,1,  // r
    3,7,6,2}; // l
  vtkCellArray *strips=vtkCellArray::New();
  for (int i=0,q=0; i<6; ++i,q+=4)
    {
    strips->InsertNextCell(4,&cellPts[q]);
    }

  surface->SetStrips(strips);
  strips->Delete();

  cellLoc->SetDataSet(surface);
  cellLoc->BuildLocator();

  surface->Delete();
}