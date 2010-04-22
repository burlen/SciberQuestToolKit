/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQRandomSeedPoints.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSQRandomSeedPoints - create a random cloud of points
// .SECTION Description
// vtkSQRandomSeedPoints is a source object that creates a user-specified number 
// of points within a specified radius about a specified center point. 
// By default location of the points is random within the sphere. It is
// also possible to generate random points only on the surface of the
// sphere.

#ifndef __vtkPointSource_h
#define __vtkPointSource_h

#include "vtkPolyDataAlgorithm.h"

class VTK_EXPORT vtkSQRandomSeedPoints : public vtkPolyDataAlgorithm
{
public:
  static vtkSQRandomSeedPoints *New();
  vtkTypeRevisionMacro(vtkSQRandomSeedPoints,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of points to generate.
  vtkSetClampMacro(NumberOfPoints,int,1,VTK_INT_MAX);
  vtkGetMacro(NumberOfPoints,int);

  // Description:
  // Set the bounding box the seed points are generated
  // inside.
  vtkSetVector6Macro(Bounds,double);
  vtkGetVector6Macro(Bounds,double);


protected:
  /// Pipeline internals.
  int FillInputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);

  vtkSQRandomSeedPoints();
  ~vtkSQRandomSeedPoints();

  int NumberOfPoints;
  double Bounds[6];

private:
  vtkSQRandomSeedPoints(const vtkSQRandomSeedPoints&);  // Not implemented.
  void operator=(const vtkSQRandomSeedPoints&);  // Not implemented.
};

#endif
