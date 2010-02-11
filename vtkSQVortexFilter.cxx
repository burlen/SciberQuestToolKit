/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#include "vtkSQVortexFilter.h"


#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkAMRBox.h"
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

//*****************************************************************************
ostream &operator<<(ostream &os, vtkAMRBox &box)
{
  box.Print(os);
  return os;
}

vtkCxxRevisionMacro(vtkSQVortexFilter, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkSQVortexFilter);

//-----------------------------------------------------------------------------
vtkSQVortexFilter::vtkSQVortexFilter()
    :
  ComputeRotation(0),
  ComputeHelicity(0),
  ComputeNormalizedHelicity(0),
  ComputeLambda(0),
  ComputeLambda2(1)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkSQVortexFilter::~vtkSQVortexFilter()
{}

// //-----------------------------------------------------------------------------
// int vtkSQVortexFilter::FillInputPortInformation(
//     int port,
//     vtkInformation *info)
// {
//   info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
//   return 1;
// }
//
// //-----------------------------------------------------------------------------
// int vtkSQVortexFilter::FillOutputPortInformation(
//     int port,
//     vtkInformation *info)
// {
//   info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
//   return 1;
// }

//-----------------------------------------------------------------------------
int vtkSQVortexFilter::RequestDataObject(
    vtkInformation* /* request */,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec)
{
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
int vtkSQVortexFilter::RequestUpdateExtent(
      vtkInformation * /* request */,
      vtkInformationVector **inInfoVec,
      vtkInformationVector *outInfoVec)
{
  vtkInformation *inInfo=inInfoVec[0]->GetInformationObject(0);
  // request a ghost layer on the input. Note PV does the domain
  // decomposition for us, we just have to add a ghost layer, then
  // trim the result so that its contained in the problem domain.
  int ext[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext);
  vtkAMRBox inputDomain(ext);
  cerr << "inputDomain=" << inputDomain << endl;

  inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),ext);
  vtkAMRBox inputExt(ext);
  inputExt.Grow(1);
  inputExt&=inputDomain;
  inputExt.GetDimensions(ext);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),ext,6);
  cerr << "inputExt=" << inputExt << endl;

  vtkInformation* outInfo=outInfoVec->GetInformationObject(0);
  // request that our output is trimmed at the domain bounds so that
  // we have a full stencil for the finite difference operations. The
  // problem domain iteself also has to be trimmed.
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext);
  vtkAMRBox outputDomain(ext);
  outputDomain.Shrink(1);
  outputDomain.GetDimensions(ext);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext,6);
  cerr << "outputDomain=" << outputDomain << endl;

  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),ext);
  vtkAMRBox outputExt(ext);
  outputExt&=outputDomain;
  outputExt.GetDimensions(ext);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),ext,6);
  cerr << "outputExt=" << outputExt << endl;

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQVortexFilter::RequestData(
    vtkInformation *req,
    vtkInformationVector **inInfoVec,
    vtkInformationVector *outInfoVec)
{
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
  int inputExt[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inputExt);
  int outputExt[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),outputExt);
  // Check that we have the ghost cells that we need (more is OK).
  vtkAMRBox inputBox(inputExt);
  vtkAMRBox outputBox(outputExt);
  outputBox.Grow(1);
  if (!inputBox.Contains(outputBox))
    {
    vtkErrorMacro(
      << "This filter requires ghost cells to function correctly. "
      << "The input must conatin the output plus a layer of ghosts. "
      << "The input is " << inputBox << ", but it must be at least "
      << outputBox << ".");
    return 1;
    }

  // NOTE You can't do a shallow copy because the array dimensions are 
  // different on output and input because of the ghost layer.
  // outData->ShallowCopy(inData);

  if (isImage)
    {
    vtkImageData *inImData=dynamic_cast<vtkImageData *>(inData);
    vtkImageData *outImData=dynamic_cast<vtkImageData *>(outData);

    // set up the output.
    double X0[3];
    inImData->GetOrigin(X0);
    outImData->SetOrigin(X0);
    outInfo->Set(vtkDataObject::ORIGIN(),X0,3);

    double dX[3];
    inImData->GetSpacing(dX);
    outImData->SetSpacing(dX);
    outInfo->Set(vtkDataObject::SPACING(),dX,3);

    int dims[3]={
      outputExt[1]-outputExt[0]+1,
      outputExt[3]-outputExt[2]+1,
      outputExt[5]-outputExt[4]+1};
    outImData->SetDimensions(dims);

    vtkDataArray *V=this->GetInputArrayToProcess(0, inImData);

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
      vtkDataArray *R=V->NewInstance();
      outImData->GetPointData()->AddArray(R);
      R->Delete();
      R->SetNumberOfComponents(3);
      R->SetNumberOfTuples(V->GetNumberOfTuples());
      string name("rot-");
      name+=V->GetName();
      R->SetName(name.c_str());
      //
      vtkFloatArray *fV=0,*fR=0;
      vtkDoubleArray *dV=0, *dR=0;
      if ( (fV=dynamic_cast<vtkFloatArray *>(V))!=NULL
        && (fR=dynamic_cast<vtkFloatArray *>(R))!=NULL)
        {
        Rotation(inputExt,outputExt,dX,fV->GetPointer(0),fR->GetPointer(0));
        }
      else
      if ( (dV=dynamic_cast<vtkDoubleArray *>(V))!=NULL
        && (dR=dynamic_cast<vtkDoubleArray *>(R))!=NULL)
        {
        Rotation(inputExt,outputExt,dX,dV->GetPointer(0),dR->GetPointer(0));
        }
      }

    // Helicity.
    if (this->ComputeHelicity)
      {
      vtkDataArray *H=V->NewInstance();
      outImData->GetPointData()->AddArray(H);
      H->Delete();
      H->SetNumberOfComponents(1);
      H->SetNumberOfTuples(V->GetNumberOfTuples());
      string name("hel-");
      name+=V->GetName();
      H->SetName(name.c_str());
      //
      vtkFloatArray *fV=0,*fH=0;
      vtkDoubleArray *dV=0, *dH=0;
      if ( (fV=dynamic_cast<vtkFloatArray *>(V))!=NULL
        && (fH=dynamic_cast<vtkFloatArray *>(H))!=NULL)
        {
        Helicity(inputExt,outputExt,dX,fV->GetPointer(0),fH->GetPointer(0));
        }
      else
      if ( (dV=dynamic_cast<vtkDoubleArray *>(V))!=NULL
        && (dH=dynamic_cast<vtkDoubleArray *>(H))!=NULL)
        {
        Helicity(inputExt,outputExt,dX,dV->GetPointer(0),dH->GetPointer(0));
        }
      }

    // Normalized Helicity.
    if (this->ComputeNormalizedHelicity)
      {
      vtkDataArray *HN=V->NewInstance();
      outImData->GetPointData()->AddArray(HN);
      HN->Delete();
      HN->SetNumberOfComponents(1);
      HN->SetNumberOfTuples(V->GetNumberOfTuples());
      string name("heln-");
      name+=V->GetName();
      HN->SetName(name.c_str());
      //
      vtkFloatArray *fV=0,*fHN=0;
      vtkDoubleArray *dV=0, *dHN=0;
      if ( (fV=dynamic_cast<vtkFloatArray *>(V))!=NULL
        && (fHN=dynamic_cast<vtkFloatArray *>(HN))!=NULL)
        {
        NormalizedHelicity(inputExt,outputExt,dX,fV->GetPointer(0),fHN->GetPointer(0));
        }
      else
      if ( (dV=dynamic_cast<vtkDoubleArray *>(V))!=NULL
        && (dHN=dynamic_cast<vtkDoubleArray *>(HN))!=NULL)
        {
        NormalizedHelicity(inputExt,outputExt,dX,dV->GetPointer(0),dHN->GetPointer(0));
        }
      }

    // Lambda-1,2,3.
    if (this->ComputeLambda)
      {
      vtkDataArray *L=V->NewInstance();
      outImData->GetPointData()->AddArray(L);
      L->Delete();
      L->SetNumberOfComponents(3);
      L->SetNumberOfTuples(V->GetNumberOfTuples());
      string name("lam-");
      name+=V->GetName();
      L->SetName(name.c_str());
      //
      vtkFloatArray *fV=0,*fL=0;
      vtkDoubleArray *dV=0, *dL=0;
      if ( (fV=dynamic_cast<vtkFloatArray *>(V))!=NULL
        && (fL=dynamic_cast<vtkFloatArray *>(L))!=NULL)
        {
        Lambda(inputExt,outputExt,dX,fV->GetPointer(0),fL->GetPointer(0));
        }
      else
      if ( (dV=dynamic_cast<vtkDoubleArray *>(V))!=NULL
        && (dL=dynamic_cast<vtkDoubleArray *>(L))!=NULL)
        {
        Lambda(inputExt,outputExt,dX,dV->GetPointer(0),dL->GetPointer(0));
        }
      }

    // Lambda-2.
    if (this->ComputeLambda2)
      {
      vtkDataArray *L2=V->NewInstance();
      outImData->GetPointData()->AddArray(L2);
      L2->Delete();
      L2->SetNumberOfComponents(1);
      L2->SetNumberOfTuples(V->GetNumberOfTuples());
      string name("lam2-");
      name+=V->GetName();
      L2->SetName(name.c_str());
      //
      vtkFloatArray *fV=0,*fL2=0;
      vtkDoubleArray *dV=0, *dL2=0;
      if ( (fV=dynamic_cast<vtkFloatArray *>(V))!=NULL
        && (fL2=dynamic_cast<vtkFloatArray *>(L2))!=NULL)
        {
        Lambda2(inputExt,outputExt,dX,fV->GetPointer(0),fL2->GetPointer(0));
        }
      else
      if ( (dV=dynamic_cast<vtkDoubleArray *>(V))!=NULL
        && (dL2=dynamic_cast<vtkDoubleArray *>(L2))!=NULL)
        {
        Lambda2(inputExt,outputExt,dX,dV->GetPointer(0),dL2->GetPointer(0));
        }
      }

    }
  else
  if (isRecti)
    {
    vtkWarningMacro("TODO : implment difference opperators on stretched grids.");
    }

 return 1;
}

//-----------------------------------------------------------------------------
void vtkSQVortexFilter::PrintSelf(ostream& os, vtkIndent indent)
{}

