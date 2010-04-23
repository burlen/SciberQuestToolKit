/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
// .NAME vtkSQSeedPointLatice - create a random cloud of points
// .SECTION Description
// vtkSQSeedPointLatice is a source object that creates a user-specified number 
// of points within a specified radius about a specified center point. 
// By default location of the points is random within the sphere. It is
// also possible to generate random points only on the surface of the
// sphere.

#ifndef __vtkSQSeedPointLatice_h
#define __vtkSQSeedPointLatice_h

#include "vtkPolyDataAlgorithm.h"

class VTK_EXPORT vtkSQSeedPointLatice : public vtkPolyDataAlgorithm
{
public:
  static vtkSQSeedPointLatice *New();
  vtkTypeRevisionMacro(vtkSQSeedPointLatice,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the bounding box the seed points are generated
  // inside.
  vtkSetVector6Macro(Bounds,double);
  vtkGetVector6Macro(Bounds,double);

  // Description:
  // Set the latice resolution in the given direction.
  vtkSetVector3Macro(NX,int);
  vtkGetVector3Macro(NX,int);

protected:
  /// Pipeline internals.
  int FillInputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);

  vtkSQSeedPointLatice();
  ~vtkSQSeedPointLatice();

  int NumberOfPoints;

  int NX[3];

  double Bounds[6];

private:
  vtkSQSeedPointLatice(const vtkSQSeedPointLatice&);  // Not implemented.
  void operator=(const vtkSQSeedPointLatice&);  // Not implemented.
};

#endif
