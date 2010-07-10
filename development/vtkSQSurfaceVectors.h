/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQSurfaceVectors.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSQSurfaceVectors - Constrains vectors to surface.
// .SECTION Description
// This filter works on point vectors.  It does not work on cell vectors yet.
// A normal is conputed for a point by averaging normals of surrounding
// 2D cells.  The vector is then constrained to be perpendicular to the normal.

#ifndef __vtkSQSurfaceVectors_h
#define __vtkSQSurfaceVectors_h

#include "vtkDataSetAlgorithm.h"

class vtkFloatArray;
class vtkIdList;

class VTK_EXPORT vtkSQSurfaceVectors : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkSQSurfaceVectors,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQSurfaceVectors *New();

  // Description:
  // This mode determines whether this filter projects vectors to
  // be perpendicular to surface or parallel to surface. It defaults
  // to parallel.
  vtkSetMacro(Mode,int);
  vtkGetMacro(Mode,int);
  //BTX
  enum {
    MODE_PARALLEL=0,
    MODE_PERP
    };
  //ETX

  // Description:
  // Project vectors onto the surface.
  void SetModeToParallel(){ this->SetMode(MODE_PARALLEL); }

  // Description:
  // Project vectors onto the surface normal.
  void SetModeToPerpendicular(){ this->SetMode(MODE_PERP);}


protected:
  vtkSQSurfaceVectors();
  ~vtkSQSurfaceVectors();

  // Usual data generation method
  virtual int RequestData(vtkInformation *,vtkInformationVector **,vtkInformationVector *);
  //virtual int RequestUpdateExtent(vtkInformation*,vtkInformationVector**,vtkInformationVector*);


private:
  int ConstraintMode;

private:
  vtkSQSurfaceVectors(const vtkSQSurfaceVectors&);  // Not implemented.
  void operator=(const vtkSQSurfaceVectors&);  // Not implemented.
};

#endif
