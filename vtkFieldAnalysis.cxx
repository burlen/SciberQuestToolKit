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

template <typename T>
void FaceDiv(int *I, double *dX, T *V, T *mV, T *div)
{
  // *hi variables are numbe of cells in the out cell centered
  // array. The in array is a point centered array of face data
  // with the last face left off.
  const int pihi=I[0]+1;
  const int pjhi=I[1]+1;
  // const int pkhi=I[2]+1;

  for (int k=0; k<I[2]; ++k) {
    for (int j=0; j<I[1]; ++j) {
      for (int i=0; i<I[0]; ++i) {
        const int c=k*I[0]*I[1]+j*I[0]+i;
        const int p=k*pihi*pjhi+j*pihi+i;

        const int vilo = 3 * (k*pihi*pjhi+j*pihi+ i   );
        const int vihi = 3 * (k*pihi*pjhi+j*pihi+(i+1));
        const int vjlo = 3 * (k*pihi*pjhi+   j *pihi+i) + 1;
        const int vjhi = 3 * (k*pihi*pjhi+(j+1)*pihi+i) + 1;
        const int vklo = 3 * (   k *pihi*pjhi+j*pihi+i) + 2;
        const int vkhi = 3 * ((k+1)*pihi*pjhi+j*pihi+i) + 2;

        //cerr << "(" << vilo << ", " << vihi << ", " << vjlo << ", " << vjhi << ", " << vklo << ", " << vkhi << ")" << endl;

        // const double modV=mV[cId];
        // (sqrt(V[vilo]*V[vilo] + V[vjlo]*V[vjlo] + V[vklo]*V[vklo])
        // + sqrt(V[vihi]*V[vihi] + V[vjhi]*V[vjhi] + V[vkhi]*V[vkhi]))/2.0;

        div[c] =(V[vihi]-V[vilo])/dX[0]/mV[p];
        div[c]+=(V[vjhi]-V[vjlo])/dX[1]/mV[p];
        div[c]+=(V[vkhi]-V[vklo])/dX[2]/mV[p];
        }
      }
    }
}

// I  -> number of points
// dX -> grid spacing triple
// V  -> vector field
// H  -> helicity
template <typename T>
void Helicity(int *I, double *dX, T *V, T *mV, T *H)
{
  const int N[3]={I[0]-1,I[1]-1,I[2]-1};
  const double dx[3]={dX[0]*2.0,dX[1]*2.0,dX[2]*2.0};

  for (int k=0; k<I[2]; ++k) {
    for (int j=0; j<I[1]; ++j) {
      for (int i=0; i<I[0]; ++i) {

        const int pId=k*I[0]*I[1]+j*I[0]+i;
        const int vpId=3*pId;

        // No ghost cells, check to see if full stenccil is available.
        if ( i==0 || j==0 || k==0 || i==N[0] || j==N[1] || k==N[2] )
          {
          // Nope, just zero the memory out.
          H[pId]=0.0;
          }
        else
          {
          // Yes, compute helicity.
          const int vilo = 3 * (k*I[0]*I[1]+j*I[0]+(i-1));
          const int vihi = 3 * (k*I[0]*I[1]+j*I[0]+(i+1));
          const int vjlo = 3 * (k*I[0]*I[1]+(j-1)*I[0]+i) + 1;
          const int vjhi = 3 * (k*I[0]*I[1]+(j+1)*I[0]+i) + 1;
          const int vklo = 3 * ((k-1)*I[0]*I[1]+j*I[0]+i) + 2;
          const int vkhi = 3 * ((k+1)*I[0]*I[1]+j*I[0]+i) + 2;

          //cerr << "(" << vilo << ", " << vihi << ", " << vjlo << ", " << vjhi << ", " << vklo << ", " << vkhi << ")" << endl;

          const double modVsq
          // = mV[pId]*mV[pId];
            = 1.0;
          // = V[vpId]*V[vpId]+V[vpId+1]*V[vpId+1]+V[vpId+2]*V[vpId+2];
          //const double modVsq=1.0;

          //      __   -> ->   -> ->
          //  H = \/ x V/|V| . V/|V|
          H[pId]
            =V[vpId  ]*((V[vkhi]-V[vklo])/dx[1]-(V[vjhi]-V[vjlo])/dx[2])/modVsq
            +V[vpId+1]*((V[vihi]-V[vilo])/dx[2]-(V[vkhi]-V[vklo])/dx[0])/modVsq
            +V[vpId+2]*((V[vjhi]-V[vjlo])/dx[0]-(V[vihi]-V[vilo])/dx[1])/modVsq;
          }
        }
      }
    }
}

// I   -> number of points
// dX  -> grid spacing triple
// V   -> vector field
// xV  -> vorticity
// mxV -> vorticity magnitude
template <typename T>
void Rotation(int *I, double *dX, T *V, T *mV, T *xV, T *mxV)
{
  const int N[3]={I[0]-1,I[1]-1,I[2]-1};
  const double dx[3]={dX[0]*2.0,dX[1]*2.0,dX[2]*2.0};

  for (int k=0; k<I[2]; ++k) {
    for (int j=0; j<I[1]; ++j) {
      for (int i=0; i<I[0]; ++i) {

        const int p  = k*I[0]*I[1]+j*I[0]+i;
        const int vi = 3*p;
        const int vj = vi + 1;
        const int vk = vi + 2;

        // No ghost cells, check to see if full stenccil is available.
        if ( i==0 || j==0 || k==0 || i==N[0] || j==N[1] || k==N[2] )
          {
          // Nope, just zero the memory out.
          xV[vi]=0.0;
          xV[vj]=0.0;
          xV[vk]=0.0;
          mxV[p]=0.0;
          }
        else
          {
          // Yes, compute rotation.
          const int vilo = 3 * (k*I[0]*I[1]+j*I[0]+(i-1));
          const int vihi = 3 * (k*I[0]*I[1]+j*I[0]+(i+1));
          const int vjlo = 3 * (k*I[0]*I[1]+(j-1)*I[0]+i) + 1;
          const int vjhi = 3 * (k*I[0]*I[1]+(j+1)*I[0]+i) + 1;
          const int vklo = 3 * ((k-1)*I[0]*I[1]+j*I[0]+i) + 2;
          const int vkhi = 3 * ((k+1)*I[0]*I[1]+j*I[0]+i) + 2;

          //cerr << "(" << vilo << ", " << vihi << ", " << vjlo << ", " << vjhi << ", " << vklo << ", " << vkhi << ")" << endl;

          const double modV
          //    = mV[p];
           = 1.0;
          // = sqrt(V[vpId]*V[vpId]+V[vpId+1]*V[vpId+1]+V[vpId+2]*V[vpId+2]);
          //const double modVsq=1.0;

          //      __   -> ->
          //  R = \/ x V/|V|
          xV[vi]=((V[vkhi]-V[vklo])/dx[1]-(V[vjhi]-V[vjlo])/dx[2])/modV;
          xV[vj]=((V[vihi]-V[vilo])/dx[2]-(V[vkhi]-V[vklo])/dx[0])/modV;
          xV[vk]=((V[vjhi]-V[vjlo])/dx[0]-(V[vihi]-V[vilo])/dx[1])/modV;
          //  ->
          // |R|
          mxV[p]=sqrt(xV[vi]*xV[vi]+xV[vj]*xV[vj]+xV[vk]*xV[vk]);
          }
        }
      }
    }
}

// I  -> number of points
// V  -> vector field
// mV -> Magnitude
template <typename T>
void Magnitude(int *I, T *V, T *mV)
{
  for (int k=0; k<I[2]; ++k) {
    for (int j=0; j<I[1]; ++j) {
      for (int i=0; i<I[0]; ++i) 
        {
        const int p  = k*I[0]*I[1]+j*I[0]+i;
        const int vi = 3*p;
        const int vj = vi + 1;
        const int vk = vi + 2;
        mV[p]=sqrt(V[vi]*V[vi]+V[vj]*V[vj]+V[vk]*V[vk]);
        }
      }
    }
}


vtkCxxRevisionMacro(vtkFieldAnalysis, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkFieldAnalysis);

//-----------------------------------------------------------------------------
vtkFieldAnalysis::vtkFieldAnalysis()
    :
  ComputeFaceDivergence(1),
  ComputeCurrentHelicity(1),
  ComputeRotation(1)
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

      // magnitude. need to scale by it.
      if (this->ComputeFaceDivergence
        || this->ComputeCurrentHelicity
        || this->ComputeRotation)
        {
        mV=vtkFloatArray::New();
        outImData->GetPointData()->AddArray(mV);
        mV->Delete();
        mV->SetNumberOfTuples(V->GetNumberOfTuples());
        string mVName("Mod-");
        mVName+=V->GetName();
        mV->SetName(mVName.c_str());
        Magnitude(P,V->GetPointer(0),mV->GetPointer(0));
        }

      // face centered divergence.
      if (this->ComputeFaceDivergence)
        {
        vtkFloatArray *divV=vtkFloatArray::New();
        outImData->GetCellData()->AddArray(divV);
        divV->Delete();
        divV->SetNumberOfTuples(C[0]*C[1]*C[2]);
        string divVName("Div-");
        divVName+=V->GetName();
        divV->SetName(divVName.c_str());
        FaceDiv(C,dX,V->GetPointer(0),mV->GetPointer(0),divV->GetPointer(0));
        }
      // (current) Helicity.
      if (this->ComputeCurrentHelicity)
        {
        vtkFloatArray *H=vtkFloatArray::New();
        outImData->GetPointData()->AddArray(H);
        H->Delete();
        H->SetNumberOfTuples(V->GetNumberOfTuples());
        string HName("Hel-");
        HName+=V->GetName();
        H->SetName(HName.c_str());
        Helicity(P,dX,V->GetPointer(0),mV->GetPointer(0),H->GetPointer(0));
        }
      // Rotation.
      if (this->ComputeRotation)
        {
        vtkFloatArray *xV=vtkFloatArray::New();
        outImData->GetPointData()->AddArray(xV);
        xV->Delete();
        xV->SetNumberOfComponents(3);
        xV->SetNumberOfTuples(V->GetNumberOfTuples());
        string xVName("Rot-");
        xVName+=V->GetName();
        xV->SetName(xVName.c_str());
        //
        vtkFloatArray *mxV=vtkFloatArray::New();
        outImData->GetPointData()->AddArray(mxV);
        mxV->Delete();
        mxV->SetNumberOfTuples(V->GetNumberOfTuples());
        string mxVName("Mod-Rot-");
        mxVName+=V->GetName();
        mxV->SetName(mxVName.c_str());
        //
        Rotation(P,dX,V->GetPointer(0),mV->GetPointer(0),xV->GetPointer(0),mxV->GetPointer(0));
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


