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
