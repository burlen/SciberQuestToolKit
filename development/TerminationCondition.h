/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __TerminationCondition_h
#define __TerminationCondition_h

#include "minmax.h"
#include "FieldLine.h"
#include "IntersectionSetColorMapper.h"
#include "vtkCellLocator.h"

#include <cmath>

#include <vector>
using std::vector;

#include "SQMacros.h"
#include "Tuple.hxx"

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
  // from points x0->x1 intersects a periodic boundary. If so
  // then p0 is set to its periodic image.
  int ApplyPeriodicBC(double *p0, double *p1);

  // Description:
  // Test for intersection of segment p0->p1 with the termination surfaces.
  // Return the surface id in case an intersection is found and 0 otehrwise.
  // The point of intersection is returned in pi and the parametric coordinate
  // of the intersection in t.
  int IntersectsTerminationSurface(
        double *p0,
        double *p1,
        double *pi);
  // Description:
  // Return boolean indicating if the specified point is outside
  // the problem domain. The two point variant updates p1 to be
  // the intersection of the segment po->p1 and the problem domain.
  int OutsideProblemDomain(double *pt);
  int OutsideProblemDomain(double *p0, double *p1);
  // Description:
  // Return boolean indicating if the field line is leaving the
  // active sub-domain domain. Lines that are leaving the active
  // sub-domain but not the problem domain can be integrated
  // further.
  int OutsideWorkingDomain(double *p);

  // Description:
  // Set the problem domain. See OutsideProblemDomain.
  void SetProblemDomain(double domain[6], int periodic[3]);

  void ClearPeriodicBC();

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
  // Adds a termination surface. See IntersectsTerminationSurface.
  void PushTerminationSurface(vtkPolyData *pd, const char *name=0);
  // Description:
  // Remove all surfaces.See IntersectsTerminationSurface.
  void ClearTerminationSurfaces();

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
  int GetFieldNullId(){ return this->TerminationSurfaces.size()+1; }
  int GetShortIntegrationId(){ return this->TerminationSurfaces.size()+2; }

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
  vtkCellLocator *PeriodicBCFaces[6];
  double WorkingDomain[6];
  vector<vtkCellLocator*> TerminationSurfaces;
  vector<string> TerminationSurfaceNames;
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
    // it's inside.
    return 0;
    }

  return 1;
}

//-----------------------------------------------------------------------------
inline
int TerminationCondition::OutsideProblemDomain(double *p0, double *p1)
{

  if((p1[0]>=this->ProblemDomain[0] && p1[0]<=this->ProblemDomain[1])
  && (p1[1]>=this->ProblemDomain[2] && p1[1]<=this->ProblemDomain[3])
  && (p1[2]>=this->ProblemDomain[4] && p1[2]<=this->ProblemDomain[5]))
    {
    // it's inside.
    return 0;
    }

  double v[3]={
      p1[0]-p0[0],
      p1[1]-p0[1],
      p1[2]-p0[2]};

  // clip p1 to the domain bounds.
  // i
  if (p1[0]<this->ProblemDomain[0])
    {
    double t=(this->ProblemDomain[0]-p0[0])/v[0];
    p1[0]=this->ProblemDomain[0];
    p1[1]=p0[1]+v[1]*t;
    p1[2]=p0[2]+v[2]*t;
    return 1;
    }
  if (p1[0]>this->ProblemDomain[1])
    {
    double t=(this->ProblemDomain[1]-p0[0])/v[0];
    p1[0]=this->ProblemDomain[1];
    p1[1]=p0[1]+v[1]*t;
    p1[2]=p0[2]+v[2]*t;
    return 1;
    }
  // j
  if (p1[1]<this->ProblemDomain[2])
    {
    double t=(this->ProblemDomain[2]-p0[1])/v[1];
    p1[0]=p0[0]+v[0]*t;
    p1[1]=this->ProblemDomain[2];
    p1[2]=p0[2]+v[2]*t;
    return 1;
    }
  if (p1[1]>this->ProblemDomain[3])
    {
    double t=(this->ProblemDomain[3]-p0[1])/v[1];
    p1[0]=p0[0]+v[0]*t;
    p1[1]=this->ProblemDomain[3];
    p1[2]=p0[2]+v[2]*t;
    return 1;
    }
  // k
  if (p1[2]<this->ProblemDomain[4])
    {
    double t=(this->ProblemDomain[4]-p0[2])/v[2];
    p1[0]=p0[0]+v[0]*t;
    p1[1]=p0[1]+v[1]*t;
    p1[2]=this->ProblemDomain[4];
    return 1;
    }
  if (p1[2]>this->ProblemDomain[5])
    {
    double t=(this->ProblemDomain[5]-p0[2])/v[2];
    p1[0]=p0[0]+v[0]*t;
    p1[1]=p0[1]+v[1]*t;
    p1[2]=this->ProblemDomain[5];
    return 1;
    }


  sqErrorMacro(cerr,"No intersection.");
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
int TerminationCondition::IntersectsTerminationSurface(
      double *p0,
      double *p1,
      double *pi)
{
  size_t nSurfaces=this->TerminationSurfaces.size();
  for (size_t i=0; i<nSurfaces; ++i)
    {
    double p[3]={0.0};
    double t=0.0;
    int c=0;
    int hitSurface
      = this->TerminationSurfaces[i]->IntersectWithLine(p0,p1,1E-6,t,pi,p,c);
    if (hitSurface)
      {
      return i+1;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
inline
int TerminationCondition::ApplyPeriodicBC(double *p0, double *p1)
{
  for (int i=0; i<6;)
    {
    vtkCellLocator *face=this->PeriodicBCFaces[i];
    // a null entry indicates non-periodic boundary
    if (face)
      {
      double t=0.0;
      double x[3]={0.0};
      double p[3]={0.0};
      int c=0;
      int hitSurface
        = this->PeriodicBCFaces[i]->IntersectWithLine(p0,p1,1E-6,t,x,p,c);
      if (hitSurface)
        {
        // replace input with the location of the intersection
        p1[0]=x[0];
        p1[1]=x[1];
        p1[2]=x[2];

        int q=i/2;      // periodic direction
        int p=(i+1)%2;  // selects opposite face

        // cerr << setw(20) << setprecision(15) << scientific << p1[q] << "->";

        // apply the periodic BC
        p1[q]=this->ProblemDomain[2*q+p];

        // If we end up exactly on the domain bounds the next interpolation
        // will fail. Teak the periodic dimension in the least significant digit
        // to be sure we are safely inside the problem domain.
        double eps=(p==0?1.0E-15:-1.0E-15);
        if (p1[q]>1.0)
            {
            int s=log10(p1[q]);
            eps*=pow(10.0,s);
            }
        p1[q]+=eps;

        // cerr << setw(20) << setprecision(15) << scientific << p1[q] << endl;

        return i;
        }
      i+=1;
      }
    else
      {
      i+=2; // periodic bc faces come in pairs.
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
