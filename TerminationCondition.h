/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef TerminationCondition_h
#define TerminationCondition_h


#include "IntersectionSetColorMapper.h"
#include "vtkCellLocator.h"
#include<vector>
using std::vector;

class vtkPolyData;


// Description:
// Defines and interface for testing whether or not
// a feild line should be terminated.
//
// 0   -> field null
// 1   -> s1
//    ...
// n   -> sn
// n+1 -> problem domain
// n+2 -> short integration
class TerminationCondition
{
public:
  TerminationCondition();
  ~TerminationCondition();

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
  // Set the problem domain. See SegmentExistProblemDomain.
  void SetProblemDomain(double domain[6]);
  // Description:
  // Set the working domain. See SegmentExitsWorkingDomain.
  void SetWorkingDomain(double domain[6]);
  // Description:
  // Adds a termination surface. See SegmentTerminates.
  void PushSurface(vtkPolyData *pd, const char *name=0);
  // Description:
  // Remove all surfaces.See SegmentTerminates.
  void ClearSurfaces();

  // Description:
  // Get a unique color using two surface ids returned
  // from SegmentTerminates(). There are two special ids
  // that may additionally be used, that of a stagnation
  // of field null retruned by GetFieldNullId and
  // , that of the problem domain returned by
  // GetProblemDoainSurfaceId().
  void InitializeColorMapper();
  int GetTerminationColor(int sId1, int sId2);
  int GetProblemDomainSurfaceId();
  int GetFieldNullId();
  int GetShortIntegrationId();
  void SqueezeColorMap(vtkIntArray *colors)
    {
    this->CMap.SqueezeColorMap(colors);
    }

private:
  // Helper, to generate a polygonal box from a set of bounds.
  void DomainToLocator(vtkCellLocator *cellLoc, double dom[6]);
  // Helper, test if a point is inside a box.
  int Outside(double box[6], double pt[3]);

private:
  double ProblemDomain[6];
  double WorkingDomain[6];
  vector<vtkCellLocator*> Surfaces;
  vector<string> SurfaceNames;
  IntersectionSetColorMapper CMap;
};

//-----------------------------------------------------------------------------
inline
int TerminationCondition::Outside(double box[6], double pt[3])
{
  if((pt[0]>=box[0] && pt[0]<=box[1])
  && (pt[1]>=box[2] && pt[1]<=box[3])
  && (pt[2]>=box[4] && pt[2]<=box[5]))
    {
    return 0;
    }
  return 1;
}

//-----------------------------------------------------------------------------
inline
int TerminationCondition::OutsideProblemDomain(double *p)
{
  return this->Outside(this->ProblemDomain,p);
}

//-----------------------------------------------------------------------------
inline
int TerminationCondition::OutsideWorkingDomain(double *p)
{
  return this->Outside(this->WorkingDomain,p);
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
      // The assumption here is that only one intersection
      // is likely and in the case where there are more than
      // one it isn't important which is identified.
      return i+1;
      }
    }
  return 0;
}

// //-----------------------------------------------------------------------------
// inline
// int TerminationCondition::SegmentExitsProblemDomain(double *p0, double *p1)
// {
//   int hitSurface=0;
//   if (this->ProblemDomain)
//     {
//     double t=0.0;
//     double x[3]={0.0};
//     double p[3]={0.0};
//     int c=0;
//     hitSurface=this->ProblemDomain->IntersectWithLine(p0,p1,1E-6,t,x,p,c);
//     }
//   return hitSurface;
// }
// 
// //-----------------------------------------------------------------------------
// inline
// int TerminationCondition::SegmentExitsWorkingDomain(double *p0, double *p1)
// {
//   int hitSurface=0;
//   if (this->WorkingDomain)
//     {
//     double t=0.0;
//     double x[3]={0.0};
//     double p[3]={0.0};
//     int c=0;
//     hitSurface=this->WorkingDomain->IntersectWithLine(p0,p1,1E-6,t,x,p,c);
//     }
//   return hitSurface;
// }

#endif
