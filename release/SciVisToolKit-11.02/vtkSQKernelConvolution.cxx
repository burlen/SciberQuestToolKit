/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#include "vtkSQKernelConvolution.h"

#include "CartesianExtent.h"
#include "postream.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include <vtkstd/string>
using vtkstd::string;

#include "Numerics.hxx"

vtkCxxRevisionMacro(vtkSQKernelConvolution, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkSQKernelConvolution);

//-----------------------------------------------------------------------------
vtkSQKernelConvolution::vtkSQKernelConvolution()
    :
  StencilWidth(3),
  KernelType(KERNEL_TYPE_GAUSIAN),
  Kernel(0),
  KernelModified(1),
  Mode(MODE_3D)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::vtkSQKernelConvolution" << endl;
  #endif

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->UpdateKernel():
}

//-----------------------------------------------------------------------------
vtkSQKernelConvolution::~vtkSQKernelConvolution()
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::~vtkSQKernelConvolution" << endl;
  #endif

  if (this->Kernel)
    {
    delete [] this->Kernel;
    this->Kernel=0;
    }
}

// //-----------------------------------------------------------------------------
// int vtkSQKernelConvolution::FillInputPortInformation(
//     int port,
//     vtkInformation *info)
// {
//   #ifdef vtkSQKernelConvolutionDEBUG
//   pCerr() << "===============================vtkSQKernelConvolution::FillInputPortInformation" << endl;
//   #endif
//
//   info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
//   return 1;
// }
//
// //-----------------------------------------------------------------------------
// int vtkSQKernelConvolution::FillOutputPortInformation(
//     int port,
//     vtkInformation *info)
// {
//   #ifdef vtkSQKernelConvolutionDEBUG
//   pCerr() << "===============================vtkSQKernelConvolution::FillOutputPortInformation" << endl;
//   #endif
//
//   info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
//   return 1;
// }


//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::SetKernelType(int type)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::SetKernelType" << endl;
  #endif

  if (type==this->KernelType)
    {
    return;
    }

  this->KernelType=type;
  this->Modified();
  this->KernelModified=1;
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::SetKernelWidth(int width)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::SetKernelWidth" << endl;
  #endif

  if (width==this->KernelWidth)
    {
    return;
    }

  if ((this->width%2)==0)
    {
    vtkErrorMacro("KernelWidth must be odd.");
    return;
    }

  this->KernelWidth=width;
  this->Modified();
  this->KernelModified=1;
}


//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::UpdateKernel()
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::UpdateKernel" << endl;
  #endif

  if (!this->KernelModified)
    {
    return;
    }

  if (this->Kernel)
    {
    delete [] this->Kernel;
    this->Kernel=0;
    }

  int size=this->KerneWidth;
  switch (this->Mode)
    {
    case MODE_3D:
      size*=this->KernelWidth;
    case MODE_2D:
      size*=this->KernelWidth;
      break;
    default:
      vtkErrorMacro("Unsupported mode " << this->Mode << ".");
    }

  this->Kernel=new float [size];
  float kernelNorm=0.0;

  if (this->KernelType==KERNEL_TYPE_GAUSIAN)
    {
    float *X=new float[this->KernelWidth];
    linspace<float>(-1.0,1.0,X);

    float B[3]={0.0,0.0,0.0};
    float a=1.0;
    float c=0.55;

    switch (this->Mode)
      {
      case MODE_2D:
        for (int j=0; j<this-KernelWidth; ++j)
          {
          for (int i=0; i<this->KernelWidth; ++i)
            {
            float x[3]={X[i],X[j],0.0};
            this->Kernel[i]=Gausian(X,a,B,c);
            kernelNorm+=this->Kernel[i];
            }
          }
        break;

      case MODE_3D:
        for (int k=0; k<this-KernelWidth; ++k)
          {
          for (int j=0; j<this-KernelWidth; ++j)
            {
            for (int i=0; i<this->KernelWidth; ++i)
              {
              float x[3]={X[i],X[j],X[k]};
              this->Kernel[i]=Gausian(X,a,B,c);
              kernelNorm+=this->Kernel[i];
              }
            }
          }
        break;

      default:
        vtkErrorMacro("Unsupported mode " << this->Mode << ".");
      }
  else
  if (this->KernelType==KERNEL_TYPE_CONSTANT)
    {
    kernelNorm=size;
    for (int i=0; i<size; ++i)
      {
      this->Kernel[i]=1.0;
      }
    }
  else
    {
    vtkErrorMacro("Unsupported KernelType " << this->KernelType << ".");
    }

  // normalize
  for (int i=0; i<size; ++i)
    {
    this->Kernel[i]/=kernelNorm;
    }


  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "Kernel=[";
  for (int i=0; i<size; ++i)
    {
    cerr << this->Kernel[i] << " ";
    }
  cerr << endl;
  #endif
}

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::RequestDataObject(
    vtkInformation* /* request */,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::RequestDataObject" << endl;
  #endif


  vtkInformation *inInfo=inInfoVec[0]->GetInformationObject(0);
  vtkDataObject *inData=inInfo->Get(vtkDataObject::DATA_OBJECT());
  const char *inputType=inData->GetClassName();

  vtkInformation *outInfo=outInfoVec->GetInformationObject(0);
  vtkDataObject *outData=outInfo->Get(vtkDataObject::DATA_OBJECT());

  if ( !outData || !outData->IsA(inputType))
    {
    outData=inData->NewInstance();
    outInfo->Set(vtkDataObject::DATA_TYPE_NAME(),inputType);
    outInfo->Set(vtkDataObject::DATA_OBJECT(),outData);
    outInfo->Set(vtkDataObject::DATA_EXTENT_TYPE(), inData->GetExtentType());
    outData->SetPipelineInformation(outInfo);
    outData->Delete();
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::RequestInformation(
      vtkInformation * /*req*/,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::RequestInformation" << endl;
  #endif
  //this->Superclass::RequestInformation(req,inInfos,outInfos);

  // We will work in a restricted problem domain so that we have
  // always a single layer of ghost cells available. To make it so
  // we'll take the upstream's domain and shrink it by half the 
  // kernel width.
  int nGhost = this->StencilWidth/2;

  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);
  CartesianExtent inputDomain;
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        inputDomain.GetData());
  switch (this->Mode)
    {
    case MODE_3D:
      inputDomain.Grow(-nGhost);
      break;

    case MODE_2D_XY:
      inputDomain.Grow(0,-nGhost);
      inputDomain.Grow(1,-nGhost);
      break;

    default:
      vtkErrorMacro("Unsupported mode " << this->Mode << ".");
      break;
    }

  vtkInformation* outInfo=outInfos->GetInformationObject(0);
  outInfo->Set(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        inputDomain.GetData(),
        6);

  // other keys that need to be coppied
  double dX[3];
  inInfo->Get(vtkDataObject::SPACING(),dX);
  outInfo->Set(vtkDataObject::SPACING(),dX,3);

  double X0[3];
  inInfo->Get(vtkDataObject::ORIGIN(),X0);
  outInfo->Set(vtkDataObject::ORIGIN(),X0,3);

  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr()
    << "WHOLE_EXTENT=" << inputDomain << endl
    << "ORIGIN=" << Tuple<double>(X0,3) << endl
    << "SPACING=" << Tuple<double>(dX,3) << endl
    << "nGhost=" << nGhost << endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::RequestUpdateExtent(
      vtkInformation * /*req*/,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::RequestUpdateExtent" << endl;
  #endif

  // We will modify the extents we request from our input so
  // that we will have a layers of ghost cells.
  int nGhost = this->StencilWidth/2;

  vtkInformation* outInfo=outInfos->GetInformationObject(0);
  CartesianExtent outputExt;
  outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        outputExt.GetData());
  switch (this->Mode)
    {
    case MODE_3D:
      outputExt.Grow(nGhost);
      break;

    case MODE_2D_XY:
      outputExt.Grow(0,nGhost);
      outputExt.Grow(1,nGhost);
      break;

    default:
      vtkErrorMacro("Unsupported mode " << this->Mode << ".");
      break;
    }

  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);
  inInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        outputExt.GetData(),
        6);

  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr()
    << "UPDATE_EXTENT=" << outputExt << endl
    << "nGhost=" << nGhost << endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::RequestData(
    vtkInformation * /*req*/,
    vtkInformationVector **inInfoVec,
    vtkInformationVector *outInfoVec)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::RequestData" << endl;
  #endif

  vtkInformation *inInfo=inInfoVec[0]->GetInformationObject(0);
  vtkDataObject *inData=inInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkInformation *outInfo=outInfoVec->GetInformationObject(0);
  vtkDataObject *outData=outInfo->Get(vtkDataObject::DATA_OBJECT());

  // Guard against empty input.
  if (!inData || !outData)
    {
    vtkErrorMacro(
      << "Empty input(" << inData << ") or output(" << outData << ") detected.");
    return 1;
    }
  // We need extent based data here.
  int isImage=inData->IsA("vtkImageData");
  int isRecti=inData->IsA("vtkrectilinearGrid");
  if (!isImage && !isRecti)
    {
    vtkErrorMacro(
      << "This filter is designed for vtkStructuredData and subclasses."
      << "You are trying to use it with " << inData->GetClassName() << ".");
    return 1;
    }

  // Get the input and output extents.
  CartesianExtent inputExt;
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        inputExt.GetData());
  CartesianExtent outputExt;
  outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        outputExt.GetData());
  CartesianExtent domainExt;
  outInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        domainExt.GetData());

  // Check that we have the ghost cells that we need (more is OK).
  int nGhost = this->StencilWidth/2;

  CartesianExtent inputBox(inputExt);
  CartesianExtent outputBox(outputExt);
  switch (this->Mode)
    {
    case MODE_3D:
      outputBox.Grow(nGhost);
      break;

    case MODE_2D_XY:
      outputBox.Grow(0,nGhost);
      outputBox.Grow(1,nGhost);
      break;

    default:
      vtkErrorMacro("Unsupported mode " << this->Mode << ".");
      break;
    }
  if (!inputBox.Contains(outputBox))
    {
    vtkErrorMacro(
      << "This filter requires ghost cells to function correctly. "
      << "The input must conatin the output plus " << nGhost
      << " layers of ghosts. The input is " << inputBox
      << ", but it must be at least "
      << outputBox << ".");
    return 1;
    }

  // NOTE You can't do a shallow copy because the array dimensions are
  // different on output and input because of the ghost layers.
  // outData->ShallowCopy(inData);

  if (isImage)
    {
    vtkImageData *inImData=dynamic_cast<vtkImageData *>(inData);
    vtkImageData *outImData=dynamic_cast<vtkImageData *>(outData);

    // set up the output.
    double X0[3];
    outInfo->Get(vtkDataObject::ORIGIN(),X0);
    outImData->SetOrigin(X0);

    double dX[3];
    outInfo->Get(vtkDataObject::SPACING(),dX);
    outImData->SetSpacing(dX);

    outImData->SetExtent(outputExt.GetData());

    int outputDims[3];
    outImData->GetDimensions(outputDims);
    int outputTups=outputDims[0]*outputDims[1]*outputDims[2];

    #ifdef vtkSQVortextFilterDEBUG
    pCerr()
      << "WHOLE_EXTENT=" << domainExt << endl
      << "UPDATE_EXTENT(input)=" << inputExt << endl
      << "UPDATE_EXTENT(output)=" << outputExt << endl
      << "ORIGIN" << Tuple<double>(X0,3) << endl
      << "SPACING" << Tuple<double>(dX,3) << endl
      << endl;
    #endif

    vtkDataArray *V=this->GetInputArrayToProcess(0,inImData);

    if (!V->IsA("vtkFloatArray") && !V->IsA("vtkDoubleArray"))
      {
      vtkErrorMacro(
        << "This filter operates on vector floating point arrays."
        << "You provided " << V->GetClassName() << ".");
      return 1;
      }

    // Rotation.
    if (this->ComputeRotation)
      {
      string name;

      vtkDataArray *Rx=V->NewInstance();
      outImData->GetPointData()->AddArray(Rx);
      Rx->Delete();
      Rx->SetNumberOfComponents(1);
      Rx->SetNumberOfTuples(outputTups);
      name="rot-x";
      name+=V->GetName();
      Rx->SetName(name.c_str());

      vtkDataArray *Ry=V->NewInstance();
      outImData->GetPointData()->AddArray(Ry);
      Ry->Delete();
      Ry->SetNumberOfComponents(1);
      Ry->SetNumberOfTuples(outputTups);
      name="rot-y";
      name+=V->GetName();
      Ry->SetName(name.c_str());

      vtkDataArray *Rz=V->NewInstance();
      outImData->GetPointData()->AddArray(Rz);
      Rz->Delete();
      Rz->SetNumberOfComponents(1);
      Rz->SetNumberOfTuples(outputTups);
      name="rot-z";
      name+=V->GetName();
      Rz->SetName(name.c_str());

      vtkDataArray *R=V->NewInstance();
      outImData->GetPointData()->AddArray(R);
      R->Delete();
      R->SetNumberOfComponents(3);
      R->SetNumberOfTuples(outputTups);
      name="rot-";
      name+=V->GetName();
      R->SetName(name.c_str());

      //
      vtkFloatArray *fV=0,*fRx=0,*fRy=0,*fRz=0,*fR=0;
      vtkDoubleArray *dV=0, *dRx=0, *dRy=0, *dRz=0, *dR=0;
      if ( (fV=dynamic_cast<vtkFloatArray *>(V))!=NULL
        && (fRx=dynamic_cast<vtkFloatArray *>(Rx))!=NULL
        && (fRy=dynamic_cast<vtkFloatArray *>(Ry))!=NULL
        && (fRz=dynamic_cast<vtkFloatArray *>(Rz))!=NULL
        && (fR=dynamic_cast<vtkFloatArray *>(R))!=NULL)
        {
        switch (this->Mode)
          {
          case MODE_3D:
            Rotation(
                inputExt.GetData(),
                outputExt.GetData(),
                dX,
                fV->GetPointer(0),
                fRx->GetPointer(0),
                fRy->GetPointer(0),
                fRz->GetPointer(0));
            Interleave(outputTups,
                fRx->GetPointer(0),
                fRy->GetPointer(0),
                fRz->GetPointer(0),
                fR->GetPointer(0));
            break;

          case MODE_2D_XY:
            RotationXY(inputExt.GetData(),outputExt.GetData(),dX,fV->GetPointer(0),fR->GetPointer(0));
            break;

          default:
            vtkErrorMacro("Unsupported mode " << this->Mode << ".");
            break;
          }
        }
      else
      if ( (dV=dynamic_cast<vtkDoubleArray *>(V))!=NULL
        && (dRx=dynamic_cast<vtkDoubleArray *>(Rx))!=NULL
        && (dRy=dynamic_cast<vtkDoubleArray *>(Ry))!=NULL
        && (dRz=dynamic_cast<vtkDoubleArray *>(Rz))!=NULL
        && (dR=dynamic_cast<vtkDoubleArray *>(R))!=NULL)
        {
        switch (this->Mode)
          {
          case MODE_3D:
            Rotation(
                inputExt.GetData(),
                outputExt.GetData(),
                dX,
                dV->GetPointer(0),
                dRx->GetPointer(0),
                dRy->GetPointer(0),
                dRz->GetPointer(0));
            Interleave(outputTups,
                dRx->GetPointer(0),
                dRy->GetPointer(0),
                dRz->GetPointer(0),
                dR->GetPointer(0));
            break;

          case MODE_2D_XY:
            RotationXY(inputExt.GetData(),outputExt.GetData(),dX,dV->GetPointer(0),dR->GetPointer(0));
            break;

          default:
            vtkErrorMacro("Unsupported mode " << this->Mode << ".");
            break;
          }
        }
      }


    // outImData->Print(cerr);
    }
  else
  if (isRecti)
    {
    vtkWarningMacro("TODO : implment difference opperators on stretched grids.");
    }

 return 1;
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::PrintSelf" << endl;
  #endif

  this->Superclass::PrintSelf(os,indent);

  // TODO

}


