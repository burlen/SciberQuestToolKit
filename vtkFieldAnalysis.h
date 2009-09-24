/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef vtkFaceCentededDivergence_h
#define vtkFaceCentededDivergence_h

#include "vtkDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;

class vtkFieldAnalysis : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkFieldAnalysis,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkFieldAnalysis *New();

  vtkSetMacro(ComputeFaceDivergence,int);
  vtkSetMacro(ComputeCurrentHelicity,int);
  vtkSetMacro(ComputeRotation,int);

protected:
  //int FillInputPortInformation(int port, vtkInformation *info);
  //int FillOutputPortInformation(int port, vtkInformation *info);
  int RequestDataObject(vtkInformation*,vtkInformationVector** inInfoVec,vtkInformationVector* outInfoVec);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  vtkFieldAnalysis();
  virtual ~vtkFieldAnalysis();

private:
  int ComputeFaceDivergence;
  int ComputeCurrentHelicity;
  int ComputeRotation;

private:
  vtkFieldAnalysis(const vtkFieldAnalysis &); // Not implemented
  void operator=(const vtkFieldAnalysis &); // Not implemented
};

#endif