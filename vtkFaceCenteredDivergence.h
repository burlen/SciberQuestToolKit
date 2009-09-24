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

class vtkFaceCenteredDivergence : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkFaceCenteredDivergence,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkFaceCenteredDivergence *New();

protected:
  //int FillInputPortInformation(int port, vtkInformation *info);
  //int FillOutputPortInformation(int port, vtkInformation *info);
  int RequestDataObject(vtkInformation*,vtkInformationVector** inInfoVec,vtkInformationVector* outInfoVec);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  vtkFaceCenteredDivergence();
  virtual ~vtkFaceCenteredDivergence();

private:
  vtkFaceCenteredDivergence(const vtkFaceCenteredDivergence &); // Not implemented
  void operator=(const vtkFaceCenteredDivergence &); // Not implemented
};

#endif
