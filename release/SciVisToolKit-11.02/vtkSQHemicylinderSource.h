/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
// .NAME vtkSQHemicylinderSource - Source/Reader that provides a polydata cylinder as 2 hemicylinders on 2 outputs.
// .SECTION Description
// Source/Reader that provides a polydata cylinder as 2 hemicylinders on 2 outputs.
//

#ifndef __vtkSQHemicylinderSource_h
#define __vtkSQHemicylinderSource_h

#include "vtkPolyDataAlgorithm.h"

#include<map>
using std::map;
using std::pair;

class vtkUnstructuredGrid;
class vtkSQOOCReader;
class vtkMultiProcessController;
class vtkInitialValueProblemSolver;
class vtkPointSet;
//BTX
class FieldLine;
class IdBlock;
class FieldTopologyMap;
class TerminationCondition;
//ETX


class VTK_EXPORT vtkSQHemicylinderSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkSQHemicylinderSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQHemicylinderSource *New();

  // Description:
  // Set/Get location of the cylinder.
  vtkSetVector3Macro(Center,double);
  vtkGetVector3Macro(Center,double);

  // Description:
  // Set/Get location of the cylinder.
  vtkSetMacro(Length,double);
  vtkGetMacro(Length,double);

  // Description:
  // Set/Get the radius of the cylinder.
  vtkSetMacro(Radius,double);
  vtkGetMacro(Radius,double);

  // Description:
  // Set/Get the resolution (number of polys) used in the output.
  vtkSetMacro(Resolution,int);
  vtkGetMacro(Resolution,int);

  // Description:
  // Set/Get descriptive names attached to each of the outputs.
  // The defaults are "north" and "south".
  vtkSetStringMacro(NorthHemicylinderName);
  vtkGetStringMacro(NorthHemicylinderName);
  vtkSetStringMacro(SouthHemicylinderName);
  vtkGetStringMacro(SouthHemicylinderName);

protected:
  vtkSQHemicylinderSource();
  ~vtkSQHemicylinderSource();

  // VTK Pipeline
  int FillInputPortInformation(int port,vtkInformation *info);
  //int FillOutputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output);

private:
  vtkSQHemicylinderSource(const vtkSQHemicylinderSource&);  // Not implemented.
  void operator=(const vtkSQHemicylinderSource&);  // Not implemented.

private:
  double Center[3];
  double Length;
  double Radius;
  int Resolution;
  char *NorthHemicylinderName;
  char *SouthHemicylinderName;
};

#endif
