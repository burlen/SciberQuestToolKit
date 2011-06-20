/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef __vtkSQImageGhosts_h
#define __vtkSQImageGhosts_h

#include "vtkDataSetAlgorithm.h"
#include "CartesianExtent.h"

class vtkInformation;
class vtkInformationVector;

class vtkSQImageGhosts : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkSQImageGhosts,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQImageGhosts *New();

  //BTX
  enum {
    MODE_3D=0,
    MODE_2D_XY,
    MODE_2D_XZ,
    MODE_2D_YZ
    };
  //ETX
  // Description:
  // Set the mode to 2D or 3D.
  vtkSetMacro(Mode,int);
  vtkGetMacro(Mode,int);

protected:
  //int FillInputPortInformation(int port, vtkInformation *info);
  //int FillOutputPortInformation(int port, vtkInformation *info);
  //int RequestDataObject(vtkInformation*,vtkInformationVector** inInfoVec,vtkInformationVector* outInfoVec);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestUpdateExtent(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  vtkSQImageGhosts();
  virtual ~vtkSQImageGhosts();

private:
  CartesianExtent Grow(const CartesianExtent &inputExt);

private:
  int WorldSize;
  int WorldRank;
  int NGhosts;
  int Mode;
  CartesianExtent ProblemDomain;

private:
  vtkSQImageGhosts(const vtkSQImageGhosts &); // Not implemented
  void operator=(const vtkSQImageGhosts &); // Not implemented
};

#endif
