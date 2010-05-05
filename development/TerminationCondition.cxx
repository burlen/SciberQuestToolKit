#include "TerminationCondition.h"

#include "vtkCellLocator.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"

#include <sstream>
using std::ostringstream;
#include "vtkPolyDataWriter.h"

#define SafeDelete(a)\
  if (a)\
    {\
    a->Delete();\
    }

//-----------------------------------------------------------------------------
TerminationCondition::TerminationCondition()
{
  this->ProblemDomain[0]=
  this->ProblemDomain[2]=
  this->ProblemDomain[4]=1;
  this->ProblemDomain[1]=
  this->ProblemDomain[3]=
  this->ProblemDomain[5]=0;

  this->WorkingDomain[0]=
  this->WorkingDomain[2]=
  this->WorkingDomain[4]=1;
  this->WorkingDomain[1]=
  this->WorkingDomain[3]=
  this->WorkingDomain[5]=0;

  this->PeriodicBCFaces[0]=
  this->PeriodicBCFaces[1]=
  this->PeriodicBCFaces[2]=
  this->PeriodicBCFaces[3]=
  this->PeriodicBCFaces[4]=
  this->PeriodicBCFaces[5]=0;
}

//-----------------------------------------------------------------------------
TerminationCondition::~TerminationCondition()
{
  this->ClearTerminationSurfaces();
  this->ClearPeriodicBC();
}

//-----------------------------------------------------------------------------
void TerminationCondition::ClearTerminationSurfaces()
{
  size_t nSurfaces=this->TerminationSurfaces.size();
  for (size_t i=0; i<nSurfaces; ++i)
    {
    SafeDelete(this->TerminationSurfaces[i]);
    }
  this->TerminationSurfaces.clear();
  this->TerminationSurfaceNames.clear();
}

//-----------------------------------------------------------------------------
void TerminationCondition::ClearPeriodicBC()
{
  for (int i=0; i<6; ++i)
    {
    if (this->PeriodicBCFaces[i])
      {
      this->PeriodicBCFaces[i]->Delete();
      this->PeriodicBCFaces[i]=0;
      }
    }
}

//-----------------------------------------------------------------------------
void TerminationCondition::SetProblemDomain(double dom[6], int periodic[3])
{
  // Copy the domain, for in/out tests
  for (int i=0; i<6; ++i)
    {
    this->ProblemDomain[i]=dom[i];
    }

  // Construct faces and coords needed to apply periodic BC's
  // if any.
  this->ClearPeriodicBC();

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

  vtkIdType cellPts[24]
    ={
    0,4,3,7,  // f
    1,5,2,6,  // b
    0,4,1,5,  // l
    3,7,2,6,  // r
    0,1,3,2,  // b
    4,5,7,6}; // t

  // in each cardinal direction
  for (int q=0; q<3; ++q)
    {
    if (periodic[q])
      {
      // for each of the two faces
      for (int p=0; p<2; ++p)
        {
        const int idx=2*q+p;

        this->PeriodicBCFaces[idx]=vtkCellLocator::New();

        vtkPolyData *face=vtkPolyData::New();
        face->SetPoints(verts);

        vtkCellArray *strips=vtkCellArray::New();
        strips->InsertNextCell(4,&cellPts[4*idx]);

        face->SetStrips(strips);
        strips->Delete();

        this->PeriodicBCFaces[idx]->SetDataSet(face);
        this->PeriodicBCFaces[idx]->BuildLocator();

        face->Delete();
        }
      }
    }
  verts->Delete();
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
void TerminationCondition::PushTerminationSurface(
      vtkPolyData *pd,
      const char *name)
{
  vtkCellLocator *cellLoc=vtkCellLocator::New();
  cellLoc->SetDataSet(pd);
  cellLoc->BuildLocator();
  this->TerminationSurfaces.push_back(cellLoc);

  if (name==0)
    {
    ostringstream os;
    os << "S" << this->TerminationSurfaces.size();
    this->TerminationSurfaceNames.push_back(os.str().c_str());
    }
  else
    {
    this->TerminationSurfaceNames.push_back(name);
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
    0,4,3,7,  // f
    1,5,2,6,  // b
    0,4,1,5,  // l
    3,7,2,6,  // r
    0,1,3,2,  // b
    4,5,7,6}; // t
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

//-----------------------------------------------------------------------------
void TerminationCondition::InitializeColorMapper()
{
  // Initialize the mapper, color scheme as follows:
  // 0   -> problem domain
  // 1   -> s1
  //    ...
  // n   -> sn
  // n+1 -> field null
  // n+2 -> short integration
  vector<string> names;
  names.push_back("domain bounds");
  names.insert(
      names.end(),
      this->TerminationSurfaceNames.begin(),
      this->TerminationSurfaceNames.end());
  names.push_back("feild null");
  names.push_back("short integration");

  size_t nSurf=this->TerminationSurfaces.size()+2; // only 2 bc problem domain is automatically included.
  this->CMap.BuildColorMap(nSurf,names);
}
