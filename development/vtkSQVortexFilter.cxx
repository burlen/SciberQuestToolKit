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
  #ifdef vtkSQVortexFilterDEBUG
  cerr << "===============================vtkSQVortexFilter::vtkSQVortexFilter" << endl;
  #endif


  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

}

//-----------------------------------------------------------------------------
vtkSQVortexFilter::~vtkSQVortexFilter()
{
  #ifdef vtkSQVortexFilterDEBUG
  cerr << "===============================vtkSQVortexFilter::~vtkSQVortexFilter" << endl;
  #endif

}

// //-----------------------------------------------------------------------------
// int vtkSQVortexFilter::FillInputPortInformation(
//     int port,
//     vtkInformation *info)
// {
//   #ifdef vtkSQVortexFilterDEBUG
//   cerr << "===============================vtkSQVortexFilter::FillInputPortInformation" << endl;
//   #endif
//
//   info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
//   return 1;
// }
//
// //-----------------------------------------------------------------------------
// int vtkSQVortexFilter::FillOutputPortInformation(
//     int port,
//     vtkInformation *info)
// {
//   #ifdef vtkSQVortexFilterDEBUG
//   cerr << "===============================vtkSQVortexFilter::FillOutputPortInformation" << endl;
//   #endif
//
//   info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
//   return 1;
// }

//-----------------------------------------------------------------------------
int vtkSQVortexFilter::RequestDataObject(
    vtkInformation* /* request */,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec)
{
  #ifdef vtkSQVortexFilterDEBUG
  cerr << "===============================vtkSQVortexFilter::RequestDataObject" << endl;
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
int vtkSQVortexFilter::RequestInformation(
      vtkInformation */*req*/,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQVortexFilterDEBUG
  cerr << "===============================vtkSQVortexFilter::RequestInformation" << endl;
  #endif
  //this->Superclass::RequestInformation(req,inInfos,outInfos);

  // We will work in a restricted problem domain so that we have
  // always a single layer of ghost cells available. To make it so
  // we'll take the upstream's domain and shrink it by 1.
  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);
  int ext[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext);
  vtkAMRBox inputDomain(ext);
  inputDomain.Shrink(1);
  inputDomain.GetDimensions(ext);
  vtkInformation* outInfo=outInfos->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext,6);

  // other keys that need to be coppied
  double dX[3];
  inInfo->Get(vtkDataObject::SPACING(),dX);
  outInfo->Set(vtkDataObject::SPACING(),dX,3);

  double X0[3];
  inInfo->Get(vtkDataObject::ORIGIN(),X0);
  outInfo->Set(vtkDataObject::ORIGIN(),X0,3);

  cerr
    << "WHOLE_EXTENT=" << Tuple<int>(ext,6) << endl
    << "ORIGIN" << Tuple<double>(X0,3) << endl
    << "SPACING" << Tuple<double>(dX,3) << endl
    << endl;

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQVortexFilter::RequestUpdateExtent(
      vtkInformation * /*req*/,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQVortexFilterDEBUG
  cerr << "===============================vtkSQVortexFilter::RequestUpdateExtent" << endl;
  #endif

  // We will modify the extents we request from our input so
  // that we will have a single layer of ghost cells.
  vtkInformation* outInfo=outInfos->GetInformationObject(0);
  int ext[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),ext);
  vtkAMRBox outputExt(ext);
  outputExt.Grow(1);
  outputExt.GetDimensions(ext);

  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),ext,6);

  cerr
    << "UPDATE_EXTENT=" << Tuple<int>(ext,6) << endl
    << endl;

  return 1;
}



//-----------------------------------------------------------------------------
int vtkSQVortexFilter::RequestData(
    vtkInformation */*req*/,
    vtkInformationVector **inInfoVec,
    vtkInformationVector *outInfoVec)
{
  #ifdef vtkSQVortexFilterDEBUG
  cerr << "===============================vtkSQVortexFilter::RequestData" << endl;
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
  int inputExt[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inputExt);
  int outputExt[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),outputExt);
  int domainExt[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),domainExt);
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
    outInfo->Get(vtkDataObject::ORIGIN(),X0);
    outImData->SetOrigin(X0);

    double dX[3];
    outInfo->Get(vtkDataObject::SPACING(),dX);
    outImData->SetSpacing(dX);

    outImData->SetExtent(outputExt);

    int outputDims[3];
    outImData->GetDimensions(outputDims);
    int outputTups=outputDims[0]*outputDims[1]*outputDims[2];

    cerr
      << "WHOLE_EXTENT=" << Tuple<int>(domainExt,6) << endl
      << "UPDATE_EXTENT(input)=" << Tuple<int>(inputExt,6) << endl
      << "UPDATE_EXTENT(output)=" << Tuple<int>(outputExt,6) << endl
      << "ORIGIN" << Tuple<double>(X0,3) << endl
      << "SPACING" << Tuple<double>(dX,3) << endl
      << endl;

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
      vtkDataArray *R=V->NewInstance();
      outImData->GetPointData()->AddArray(R);
      R->Delete();
      R->SetNumberOfComponents(3);
      R->SetNumberOfTuples(outputTups);
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
      H->SetNumberOfTuples(outputTups);
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
      HN->SetNumberOfTuples(outputTups);
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
      L->SetNumberOfTuples(outputTups);
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
      L2->SetNumberOfTuples(outputTups);
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
void vtkSQVortexFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef vtkSQVortexFilterDEBUG
  cerr << "===============================vtkSQVortexFilter::PrintSelf" << endl;
  #endif

  this->Superclass::PrintSelf(os,indent);

  // TODO

}


