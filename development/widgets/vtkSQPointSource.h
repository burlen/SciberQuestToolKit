/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQPointSource.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSQPointSource - create a random cloud of points
// .SECTION Description
// vtkSQPointSource is a source object that creates a user-specified number 
// of points within a specified radius about a specified center point. 
// By default location of the points is random within the sphere. It is
// also possible to generate random points only on the surface of the
// sphere.

#ifndef __vtkSQPointSource_h
#define __vtkSQPointSource_h

#include "vtkPolyDataAlgorithm.h"

#define VTK_POINT_UNIFORM   1
#define VTK_POINT_SHELL     0

class VTK_GRAPHICS_EXPORT vtkSQPointSource : public vtkPolyDataAlgorithm 
{
public:
  static vtkSQPointSource *New();
  vtkTypeRevisionMacro(vtkSQPointSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the number of points to generate.
  vtkSetClampMacro(NumberOfPoints,vtkIdType,1,VTK_LARGE_ID);
  vtkGetMacro(NumberOfPoints,vtkIdType);

  // Description:
  // Set the center of the point cloud.
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);

  // Description:
  // Set the radius of the point cloud.  If you are
  // generating a Gaussian distribution, then this is
  // the standard deviation for each of x, y, and z.
  vtkSetClampMacro(Radius,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(Radius,double);

  // Description:
  // Specify the distribution to use.  The default is a
  // uniform distribution.  The shell distribution produces
  // random points on the surface of the sphere, none in the interior.
  vtkSetMacro(Distribution,int);
  void SetDistributionToUniform() {
    this->SetDistribution(VTK_POINT_UNIFORM);};
  void SetDistributionToShell() {
    this->SetDistribution(VTK_POINT_SHELL);};
  vtkGetMacro(Distribution,int);

protected:
  vtkSQPointSource(vtkIdType numPts=10);
  ~vtkSQPointSource() {};

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkIdType NumberOfPoints;
  double Center[3];
  double Radius;
  int Distribution;

private:
  vtkSQPointSource(const vtkSQPointSource&);  // Not implemented.
  void operator=(const vtkSQPointSource&);  // Not implemented.
};

#endif
