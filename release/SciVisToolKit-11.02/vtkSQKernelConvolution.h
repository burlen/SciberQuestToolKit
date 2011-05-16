/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef __vtkSQKernelConvolution_h
#define __vtkSQKernelConvolution_h

#include "vtkDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;

class vtkSQKernelConvolution : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkSQKernelConvolution,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQKernelConvolution *New();

  //BTX
  enum {
    MODE_3D=0,
    MODE_2D_XY,
    MODE_2D_XZ,
    MODE_2D_YZ
    };
  //ETX
  // Description:
  // Set the mode to 2 or 3D.
  vtkSetMacro(Mode,int);
  vtkGetMacro(Mode,int);

  //BTX
  enum {
    KERNEL_TYPE_GAUSIAN=0,
    KERNEL_TYPE_AVERAGE=1
    };
  //ETX
  // Description:
  // Select a kernel for the convolution.
  vtkSetMacro(KernelType,int);
  vtkGetMacro(KernelType,int);

  // Description:
  // Set the stencil width, must be an odd integer, bound bellow by 3
  // and above by the size of the smallest block
  vtkSetMacro(int,StencilWidth);
  vtkGetMacro(int,StencilWidth);

protected:
  //int FillInputPortInformation(int port, vtkInformation *info);
  //int FillOutputPortInformation(int port, vtkInformation *info);
  int RequestDataObject(vtkInformation*,vtkInformationVector** inInfoVec,vtkInformationVector* outInfoVec);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestUpdateExtent(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  vtkSQKernelConvolution();
  virtual ~vtkSQKernelConvolution();

private:
  int StencilWidth;
  int KernelType;
  float *Kernel;
  int KernelModified;
  //
  int OutputExt[6];
  int DomainExt[6];
  //
  int Mode;

private:
  vtkSQKernelConvolution(const vtkSQKernelConvolution &); // Not implemented
  void operator=(const vtkSQKernelConvolution &); // Not implemented
};

#endif
