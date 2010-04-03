/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#include "vtkFieldAnalysis.h"

#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include <vtkstd/string>
using vtkstd::string;

#include "Numerics.hxx"

vtkCxxRevisionMacro(vtkFieldAnalysis, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkFieldAnalysis);

//-----------------------------------------------------------------------------
vtkFieldAnalysis::vtkFieldAnalysis()
    :
  ComputeFaceDivergence(1)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkFieldAnalysis::~vtkFieldAnalysis()
{}

// //-----------------------------------------------------------------------------
// int vtkFieldAnalysis::FillInputPortInformation(
//     int port,
//     vtkInformation *info)
// {
//   info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
//   return 1;
// }
//
// //-----------------------------------------------------------------------------
// int vtkFieldAnalysis::FillOutputPortInformation(
//     int port,
//     vtkInformation *info)
// {
//   info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
//   return 1;
// }

//-----------------------------------------------------------------------------
int vtkFieldAnalysis::RequestDataObject(
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
int vtkFieldAnalysis::RequestData(
    vtkInformation *req,
    vtkInformationVector **inInfoVec,
    vtkInformationVector *outInfoVec)
{
  vtkInformation *inInfo=inInfoVec[0]->GetInformationObject(0);
  vtkDataObject *inData=inInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkInformation *outInfo=outInfoVec->GetInformationObject(0);
  vtkDataObject *outData=outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (!inData || !outData)
    {
    vtkWarningMacro("Filter has not been properly configured. Aborting.");
    return 1;
    }

  int isImage=inData->IsA("vtkImageData");
  int isRecti=inData->IsA("vtkrectilinearGrid");

  if (!isImage && !isRecti)
    {
    vtkWarningMacro("Unexpected type vector array. Aborting.");
    return 1;
    }

  outData->ShallowCopy(inData);

  if (isImage)
    {
    vtkImageData *inImData=dynamic_cast<vtkImageData *>(inData);
    vtkImageData *outImData=dynamic_cast<vtkImageData *>(outData);

    int P[3];     // point dimensions
    int C[3];     // cell dimensions
    inImData->GetDimensions(P);
    C[0]=P[0]-1;
    C[1]=P[1]-1;
    C[2]=P[2]-1;
    double dX[3]; // grid spacing
    inImData->GetSpacing(dX);

    vtkDataArray *da=this->GetInputArrayToProcess(0, inImData);

    int isFloat=da->IsA("vtkFloatArray");
    int isDouble=da->IsA("vtkDoubleArray");
    if (!isFloat && !isDouble)
      {
      vtkWarningMacro("Floating point data required. Aborting.");
      return 1;
      }

    if (isFloat)
      {
      vtkFloatArray *V=dynamic_cast<vtkFloatArray *>(da);
      vtkFloatArray *mV;
      // face centered divergence.
      if (this->ComputeFaceDivergence)
        {
        mV=vtkFloatArray::New();
        outImData->GetPointData()->AddArray(mV);
        mV->Delete();
        mV->SetNumberOfTuples(V->GetNumberOfTuples());
        string mVName("Mod-");
        mVName+=V->GetName();
        mV->SetName(mVName.c_str());
        Magnitude(P,V->GetPointer(0),mV->GetPointer(0));

        vtkFloatArray *divV=vtkFloatArray::New();
        outImData->GetCellData()->AddArray(divV);
        divV->Delete();
        divV->SetNumberOfTuples(C[0]*C[1]*C[2]);
        string divVName("Div-");
        divVName+=V->GetName();
        divV->SetName(divVName.c_str());
        FaceDiv(C,dX,V->GetPointer(0),mV->GetPointer(0),divV->GetPointer(0));
        }
      }
    else
    if (isDouble)
      {
      vtkWarningMacro("TODO : handle double types.");

      }
    }
  else
  if (isRecti)
    {
    vtkWarningMacro("TODO : implment divregence for stretched grids.");
    }

 return 1;
}

//-----------------------------------------------------------------------------
void vtkFieldAnalysis::PrintSelf(ostream& os, vtkIndent indent)
{}


