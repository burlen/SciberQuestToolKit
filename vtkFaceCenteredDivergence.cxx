#include "vtkFaceCenteredDivergence.h"

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

template <typename T>
void Div(int *I, double *dX, T *V, T *div)
{
  // *hi variables are numbe of cells in the out cell centered
  // array. The in array is a point centered array of face data
  // with the last face left off.
  const int pihi=I[0]+1;
  const int pjhi=I[1]+1;
  const int pkhi=I[2]+1;

  for (int k=0; k<I[2]; ++k) {
    for (int j=0; j<I[1]; ++j) {
      for (int i=0; i<I[0]; ++i) {
        const int cId=k*I[0]*I[1]+j*I[0]+i;

        const int vilo = 3 * (k*pihi*pjhi+j*pihi+ i   );
        const int vihi = 3 * (k*pihi*pjhi+j*pihi+(i+1));
        const int vjlo = 3 * (k*pihi*pjhi+   j *pihi+i) + 1;
        const int vjhi = 3 * (k*pihi*pjhi+(j+1)*pihi+i) + 1;
        const int vklo = 3 * (   k *pihi*pjhi+j*pihi+i) + 2;
        const int vkhi = 3 * ((k+1)*pihi*pjhi+j*pihi+i) + 2;

        //cerr << "(" << vilo << ", " << vihi << ", " << vjlo << ", " << vjhi << ", " << vklo << ", " << vkhi << ")" << endl;

        double modV =
          (sqrt(V[vilo]*V[vilo] + V[vjlo]*V[vjlo] + V[vklo]*V[vklo])
          + sqrt(V[vihi]*V[vihi] + V[vjhi]*V[vjhi] + V[vkhi]*V[vkhi]))/2.0;

        div[cId] =(V[vihi]-V[vilo])/dX[0]/modV;
        div[cId]+=(V[vjhi]-V[vjlo])/dX[1]/modV;
        div[cId]+=(V[vkhi]-V[vklo])/dX[2]/modV;
        }
      }
    }
}

vtkCxxRevisionMacro(vtkFaceCenteredDivergence, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkFaceCenteredDivergence);

//-----------------------------------------------------------------------------
vtkFaceCenteredDivergence::vtkFaceCenteredDivergence()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkFaceCenteredDivergence::~vtkFaceCenteredDivergence()
{}

// //-----------------------------------------------------------------------------
// int vtkFaceCenteredDivergence::FillInputPortInformation(
//     int port,
//     vtkInformation *info)
// {
//   info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
//   return 1;
// }
// 
// //-----------------------------------------------------------------------------
// int vtkFaceCenteredDivergence::FillOutputPortInformation(
//     int port,
//     vtkInformation *info)
// {
//   info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
//   return 1;
// }

//-----------------------------------------------------------------------------
int vtkFaceCenteredDivergence::RequestDataObject(
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
int vtkFaceCenteredDivergence::RequestData(
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

    int I[3];
    inImData->GetDimensions(I);
    I[0]--; // point to cell dims
    I[1]--;
    I[2]--;

    double dX[3];
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
      vtkFloatArray *divV=vtkFloatArray::New();
      outImData->GetCellData()->AddArray(divV);
      divV->Delete();
      divV->SetNumberOfTuples(I[0]*I[1]*I[2]);
      string divVName("Div-");
      divVName+=V->GetName();
      divV->SetName(divVName.c_str());
      Div(I,dX,V->GetPointer(0),divV->GetPointer(0));
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
void vtkFaceCenteredDivergence::PrintSelf(ostream& os, vtkIndent indent)
{}


