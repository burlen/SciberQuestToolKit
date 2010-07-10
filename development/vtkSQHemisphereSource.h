/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
// .NAME vtkSQHemisphereSource - Source/Reader that provides a polydata sphere as 2 hemispheres on 2 outputs.
// .SECTION Description
// Source/Reader that provides a polydata sphere as 2 hemispheres on 2 outputs.
//

#ifndef __vtkSQHemisphereSource_h
#define __vtkSQHemisphereSource_h

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


class VTK_EXPORT vtkSQHemisphereSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkSQHemisphereSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQHemisphereSource *New();

  // Description:
  // Set/Get location of the sphere.
  vtkSetVector3Macro(Center,double);
  vtkGetVector3Macro(Center,double);

  // Description:
  // Set/Get the radius of the sphere.
  vtkSetMacro(Radius,double);
  vtkGetMacro(Radius,double);

  // Description:
  // Set/Get the resolution (number of polys) used in the output.
  vtkSetMacro(Resolution,int);
  vtkGetMacro(Resolution,int);

  // Description:
  // Set/Get descriptive names attached to each of the outputs.
  // The defaults are "north" and "south".
  vtkSetStringMacro(NorthHemisphereName);
  vtkGetStringMacro(NorthHemisphereName);
  vtkSetStringMacro(SouthHemisphereName);
  vtkGetStringMacro(SouthHemisphereName);

protected:
  vtkSQHemisphereSource();
  ~vtkSQHemisphereSource();

  // VTK Pipeline
  int FillInputPortInformation(int port,vtkInformation *info);
  //int FillOutputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output);

private:
  vtkSQHemisphereSource(const vtkSQHemisphereSource&);  // Not implemented.
  void operator=(const vtkSQHemisphereSource&);  // Not implemented.

private:
  double Center[3];
  double Radius;
  int Resolution;
  char *NorthHemisphereName;
  char *SouthHemisphereName;
};

#endif
