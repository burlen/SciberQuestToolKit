/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/ 
#ifndef vtkFieldTopologyAnalyzer_h
#define vtkFieldTopologyAnalyzer_h

#include "vtkDataSetAlgorithm.h"

class vtkImplicitFunction;
class vtkCutter;
class vtkInformation;
class vtkMultiProcessController;

/**
/// Class for visulaizing field topology using John Dorelli's methods.
Field lines will be traced fro each point of a surface resulting from
a slice of the input dataset. The cut plane is defined by setting an
implicit function vis the class's SetCutFunction method. The number of
seed points is controled by the SetCutResolution method, the polygon
created by slicing the bounds of the input is triangulated and subdivided
CutResolution many times. There are (as many as) 4 outputs produced.
Output 1 is the seed points. These have an array that identifies which
of the surfaces (present on input port 1) the field line from the seed
point intersects. Only a single intersection in each direction is allowed.
The second and third output are the field lines. The forth output is the 
intersection points. The second through foooourth outputs can be activated by
compiling with -DvtkFieldTopolgyAnalyzerALLOUT in the compile coomand.
*/
class VTK_EXPORT vtkFieldTopologyAnalyzer : public vtkDataSetAlgorithm
{
private:
  vtkFieldTopologyAnalyzer(const vtkFieldTopologyAnalyzer &); // Not implemented
  void operator=(const vtkFieldTopologyAnalyzer &); // Not implemented

public:
  static vtkFieldTopologyAnalyzer *New();
  vtkTypeRevisionMacro(vtkFieldTopologyAnalyzer,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// PARAVIEW interface stuff
  // Description
  // Specify the implicit function to perform the cutting.
  virtual void SetCutFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(CutFunction,vtkImplicitFunction);

  // Description:
  // Set/Get the  resolution used when creating field line
  // seed source.
  vtkSetMacro(CutResolution,int);
  //vtkGetMacro(CutResolution,int);

  // Description:
  // Specify limits on field line length and integration.
  vtkSetMacro(MaxLineLength,double);
  vtkSetMacro(MaxNumberOfSteps,int);

  // Description:
  // Set the integration step size (as a fraction of cell size) used 
  // during field line integration.
  vtkSetMacro(StepSize,double);

  // Description:
  // Specify the dataset with the vector field to analyze.
  void AddDatasetInputConnection(vtkAlgorithmOutput* algOutput);
  void ClearDatasetInputConnections();
  // Description:
  // Specify a set of surfaces to use.
  void AddBoundaryInputConnection(vtkAlgorithmOutput* algOutput);
  void ClearBoundaryInputConnections();

  // Description:
  // Override GetMTime because we refer to vtkImplicitFunction.
  unsigned long GetMTime();

protected:
  /// Pipeline internals.
  int FillInputPortInformation(int port,vtkInformation *info);
  int FillOutputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output);
  int RequestUpdateExtent(vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output);

  vtkFieldTopologyAnalyzer();
  ~vtkFieldTopologyAnalyzer();

private:
  int MaxNumberOfSteps;
  double MaxLineLength;
  double StepSize;
  vtkImplicitFunction *CutFunction;
  vtkCutter *Cutter;
  int CutResolution;
  vtkMultiProcessController *Controller;
};

#endif
