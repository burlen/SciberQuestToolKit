#include "TerminationCondition.h"

#include "vtkCellLocator.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"

#define SafeDelete(a)\
  if (a)\
    {\
    a->Delete();\
    }

//-----------------------------------------------------------------------------
TerminationCondition::TerminationCondition()
{
  ProblemDomain[0]=ProblemDomain[2]=ProblemDomain[4]=1;
  ProblemDomain[1]=ProblemDomain[3]=ProblemDomain[5]=0;
  WorkingDomain[0]=WorkingDomain[2]=WorkingDomain[4]=1;
  WorkingDomain[1]=WorkingDomain[3]=WorkingDomain[5]=0;
}

//-----------------------------------------------------------------------------
TerminationCondition::~TerminationCondition()
{
  this->ClearSurfaces();
}

//-----------------------------------------------------------------------------
void TerminationCondition::ClearSurfaces()
{
  size_t nSurfaces=this->Surfaces.size();
  for (size_t i=0; i<nSurfaces; ++i)
    {
    SafeDelete(this->Surfaces[i]);
    }
  this->Surfaces.clear();
  this->SurfaceNames.clear();
}

//-----------------------------------------------------------------------------
void TerminationCondition::SetProblemDomain(double dom[6])
{
  for (int i=0; i<6; ++i)
    {
    this->ProblemDomain[i]=dom[i];
    }
}

//-----------------------------------------------------------------------------
void TerminationCondition::SetWorkingDomain(double dom[6])
{
  for (int i=0; i<6; ++i)
    {
    this->WorkingDomain[i]=dom[i];
    }
}

//-----------------------------------------------------------------------------
void TerminationCondition::PushSurface(vtkPolyData *pd, const char *name)
{
  vtkCellLocator *cellLoc=vtkCellLocator::New();
  cellLoc->SetDataSet(pd);
  cellLoc->BuildLocator();
  this->Surfaces.push_back(cellLoc);

  if (name==0)
    {
    ostringstream os;
    os << "S" << this->Surfaces.size();
    this->SurfaceNames.push_back(os.str().c_str());
    }
  else
    {
    this->SurfaceNames.push_back(name);
    }
}

//-----------------------------------------------------------------------------
int TerminationCondition::GetTerminationColor(FieldLine *line)
{
  return
    this->GetTerminationColor(line->GetForwardTerminator(),line->GetBackwardTerminator());
}

//-----------------------------------------------------------------------------
inline
int TerminationCondition::GetTerminationColor(int s1, int s2)
{
  return this->CMap.LookupColor(s1,s2);
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
