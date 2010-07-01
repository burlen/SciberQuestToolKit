/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
// .NAME vtkSQVolumetricSource -Create volume of hexahedral cells.
// .SECTION Description
// Creates a volume composed of hexahedra cells on a latice.
// This is the 3D counterpart to the plane source.

#ifndef __vtkSQVolumetricSource_h
#define __vtkSQVolumetricSource_h

#include "vtkUnstructuredGridAlgorithm.h"

class VTK_EXPORT vtkSQVolumetricSource : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkSQVolumetricSource *New();
  vtkTypeRevisionMacro(vtkSQVolumetricSource,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the points defining edges of a 3D quarilateral.
  vtkSetVector3Macro(Origin,double);
  vtkGetVector3Macro(Origin,double);

  vtkSetVector3Macro(Point1,double);
  vtkGetVector3Macro(Point1,double);

  vtkSetVector3Macro(Point2,double);
  vtkGetVector3Macro(Point2,double);

  vtkSetVector3Macro(Point3,double);
  vtkGetVector3Macro(Point3,double);

  // Description:
  // Set the latice resolution in the given direction.
  vtkSetVector3Macro(NCells,int);
  vtkGetVector3Macro(NCells,int);

protected:
  /// Pipeline internals.
  //int FillInputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);

  vtkSQVolumetricSource();
  ~vtkSQVolumetricSource();

private:
  double Origin[3];
  double Point1[3];
  double Point2[3];
  double Point3[3];
  int NCells[3];

private:
  vtkSQVolumetricSource(const vtkSQVolumetricSource&);  // Not implemented.
  void operator=(const vtkSQVolumetricSource&);  // Not implemented.
};

#endif
