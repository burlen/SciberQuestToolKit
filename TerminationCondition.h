/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef TerminationCondition_h
#define TerminationCondition_h

#include "FieldLine.h"
#include "IntersectionSetColorMapper.h"
#include "vtkCellLocator.h"
#include<vector>
using std::vector;

class vtkPolyData;

// Description:
// Defines and interface for testing whether or not
// a feild line should be terminated.

class TerminationCondition
{
public:
  TerminationCondition();
  virtual ~TerminationCondition();

  // Description:
  // Return a value of zero if the field line with last segment 
  // from points x0->x1  should be terminated now. A positive value
  // will be an application defined termination code that can be used
  // as a color in a rendering.
  int SegmentTerminates(double *p0, double *p1);
  // Description:
  // Return boolean indicating if the field line is leaving the
  // problem domain.
  int OutsideProblemDomain(double *p);
  // Description:
  // Return boolean indicating if the field line is leaving the
  // active sub-domain domain. Lines that are leaving the active
  // sub-domain but not the problem domain can be integrated
  // further.
  int OutsideWorkingDomain(double *p);

  // Description:
  // Set the problem domain. See OutsideProblemDomain.
  void SetProblemDomain(double domain[6]);
  // Description:
  // Set the problem domain to an empty domain. See OutsideProblemDomain.
  void ResetProblemDomain()
    {
    this->ResetDomain(this->ProblemDomain);
    }
  // Description:
  // Set the working domain. See OutsideWorkingDomain.
  void SetWorkingDomain(double domain[6]);
  // Description:
  // Set the working domain to an empty domain. See OutsideWorkingDomain.
  void ResetWorkingDomain()
    {
    this->ResetDomain(this->WorkingDomain);
    }
  // Description:
  // Adds a termination surface. See SegmentTerminates.
  void PushSurface(vtkPolyData *pd, const char *name=0);
  // Description:
  // Remove all surfaces.See SegmentTerminates.
  void ClearSurfaces();

  // Description:
  // Convert implementation defined surface ids into a unique color.
  int GetTerminationColor(FieldLine *line);
  int GetTerminationColor(int sId1, int sId2);

  // Description:
  // Build the color mapper with the folowing scheme:
  //
  // 0   -> field null
  // 1   -> s1
  //    ...
  // n   -> sn
  // n+1 -> problem domain
  // n+2 -> short integration
  void InitializeColorMapper();
  // Description:
  // Return the indentifier for the special termination cases.
  int GetProblemDomainSurfaceId(){ return 0; }
  int GetFieldNullId(){ return this->Surfaces.size()+1; }
  int GetShortIntegrationId(){ return this->Surfaces.size()+2; }

  // Description:
  // Eliminate unused colors from the the lookup table and send
  // a legend to the terminal on proc 0. This requires global
  // communication all processes must be involved.
  void SqueezeColorMap(vtkIntArray *colors)
    {
    this->CMap.SqueezeColorMap(colors);
    }
  // Description:
  // Send a legend of the used colors to the terminal on proc 0. This
  // requires global communication all processes must be involved.
  void PrintColorMap()
    {
    this->CMap.PrintUsed();
    }

private:
  // Helper, to generate a polygonal box from a set of bounds.
  void DomainToLocator(vtkCellLocator *cellLoc, double dom[6]);
  // Reset domain.
  void ResetDomain(double dom[6]);

private:
  double ProblemDomain[6];
  double WorkingDomain[6];
  vector<vtkCellLocator*> Surfaces;
  vector<string> SurfaceNames;
  IntersectionSetColorMapper CMap;
};

//-----------------------------------------------------------------------------
inline
int TerminationCondition::OutsideProblemDomain(double *pt)
{

  if((pt[0]>=this->ProblemDomain[0] && pt[0]<=this->ProblemDomain[1])
  && (pt[1]>=this->ProblemDomain[2] && pt[1]<=this->ProblemDomain[3])
  && (pt[2]>=this->ProblemDomain[4] && pt[2]<=this->ProblemDomain[5]))
    {
    return 0;
    }
  // clip
  pt[0]=max(pt[0],this->ProblemDomain[0]);
  pt[0]=min(pt[0],this->ProblemDomain[1]);
  pt[1]=max(pt[1],this->ProblemDomain[2]);
  pt[1]=min(pt[1],this->ProblemDomain[3]);
  pt[2]=max(pt[2],this->ProblemDomain[4]);
  pt[2]=min(pt[2],this->ProblemDomain[5]);
  return 1;
}

//-----------------------------------------------------------------------------
inline
int TerminationCondition::OutsideWorkingDomain(double *pt)
{
  if((pt[0]>=this->WorkingDomain[0] && pt[0]<=this->WorkingDomain[1])
  && (pt[1]>=this->WorkingDomain[2] && pt[1]<=this->WorkingDomain[3])
  && (pt[2]>=this->WorkingDomain[4] && pt[2]<=this->WorkingDomain[5]))
    {
    return 0;
    }
  return 1;
}

//-----------------------------------------------------------------------------
inline
int TerminationCondition::SegmentTerminates(double *p0, double *p1)
{
  size_t nSurfaces=this->Surfaces.size();
  for (size_t i=0; i<nSurfaces; ++i)
    {
    double t=0.0;
    double x[3]={0.0};
    double p[3]={0.0};
    int c=0;
    int hitSurface=this->Surfaces[i]->IntersectWithLine(p0,p1,1E-6,t,x,p,c);
    if (hitSurface)
      {
      // replace input with the location of the intersection
      p1[0]=x[0];
      p1[1]=x[1];
      p1[2]=x[2];
      return i+1;
      }
    }
  return 0;
}

//-------------------------------------------------------------------------------
inline
void TerminationCondition::ResetDomain(double dom[6])
{
  dom[0]=dom[2]=dom[4]=1;
  dom[1]=dom[3]=dom[5]=0;
}

#endif
