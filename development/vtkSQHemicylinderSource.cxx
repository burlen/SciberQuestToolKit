/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/

#include "vtkSQHemicylinderSource.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPolyData.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkPoints.h"

#include "vtkSQMetaDataKeys.h"
#include "GDAMetaDataKeys.h"

#include <math.h>

#if defined _WIN32
  #define M_PI 3.14151692358979
#endif

//*****************************************************************************
void GenerateHemicylinder(
      vtkPolyData *output,
      double center[3],
      double radius,
      double length,
      int resolution,
      double theta0)
{
  vtkFloatArray *X=vtkFloatArray::New();
  X->SetNumberOfComponents(3);
  X->SetNumberOfTuples(2*(resolution+1));
  float *pX=X->GetPointer(0);

  vtkPoints *pts=vtkPoints::New();
  pts->SetData(X);
  X->Delete();

  output->SetPoints(pts);
  pts->Delete();

  vtkIdTypeArray *ia=vtkIdTypeArray::New();
  ia->SetNumberOfComponents(1);
  ia->SetNumberOfTuples(5*resolution);
  vtkIdType *pIa=ia->GetPointer(0);

  vtkCellArray *polys=vtkCellArray::New();
  polys->SetCells(resolution,ia);
  ia->Delete();

  output->SetPolys(polys);
  polys->Delete();

  double theta=theta0;
  double dtheta=M_PI/resolution;

  double z0=center[2]-length/2.0;
  double z1=center[2]+length/2.0;

  pX[0]=pX[3]=center[0]+radius*cos(theta);
  pX[1]=pX[4]=center[1]+radius*sin(theta);
  pX[2]=z0;
  pX[5]=z1;
  pX+=6;
  theta+=dtheta;

  for (int i=1; i<=resolution; ++i)
    {
    // add two points
    pX[0]=pX[3]=center[0]+radius*cos(theta);
    pX[1]=pX[4]=center[1]+radius*sin(theta);
    pX[2]=z0;
    pX[5]=z1;
    pX+=6;
    theta+=dtheta;
    // describe a quad on the surface of the cylinder.
    int pid=2*(i-1);
    pIa[0]=4;
    pIa[1]=pid;
    pIa[2]=pid+1;
    pIa[3]=pid+3;
    pIa[4]=pid+2;
    pIa+=5;
    }
}


vtkCxxRevisionMacro(vtkSQHemicylinderSource, "$Revision: 1.70 $");
vtkStandardNewMacro(vtkSQHemicylinderSource);

//----------------------------------------------------------------------------
vtkSQHemicylinderSource::vtkSQHemicylinderSource()
          :
      NorthHemicylinderName(0),
      SouthHemicylinderName(0)
{
  #ifdef vtkSQHemicylinderSourceDEBUG
  cerr << "===============================vtkSQHemicylinderSource::vtkSQHemicylinderSource" << endl;
  #endif
  this->Radius=1.0;

  this->Center[0]=0.0;
  this->Center[1]=0.0;
  this->Center[2]=0.0;

  this->Length=1.0;

  this->Resolution=32;

  this->SetNorthHemicylinderName("North");
  this->SetSouthHemicylinderName("South");

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(2);
}

//----------------------------------------------------------------------------
vtkSQHemicylinderSource::~vtkSQHemicylinderSource()
{
  #ifdef vtkSQHemicylinderSourceDEBUG
  cerr << "===============================vtkSQHemicylinderSource::~vtkSQHemicylinderSource" << endl;
  #endif
  this->SetNorthHemicylinderName(0);
  this->SetSouthHemicylinderName(0);
}

//----------------------------------------------------------------------------
int vtkSQHemicylinderSource::FillInputPortInformation(
      int /*port*/,
      vtkInformation *info)
{
  #ifdef vtkSQHemicylinderSourceDEBUG
  cerr << "===============================vtkSQHemicylinderSource::FillInputPortInformation" << endl;
  #endif
  // The input is optional, if used we'll look for some keys that define
  // the sphere's attriibutes coming from upstram (e.g from a reader).
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(),1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSQHemicylinderSource::RequestInformation(
      vtkInformation* /*req*/,
      vtkInformationVector** inInfos,
      vtkInformationVector* outInfos)
{
  #ifdef vtkSQHemicylinderSourceDEBUG
  cerr << "===============================vtkSQHemicylinderSource::RequestInformation" << endl;
  #endif
  // The GDA meta data reader will insert information about
  // the center and radius of the dipole. If its there we'll
  // initialize the object from these.

  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);
  if (inInfo && inInfo->Has(GDAMetaDataKeys::DIPOLE_CENTER()))
      {
      inInfo->Get(GDAMetaDataKeys::DIPOLE_CENTER(),this->Center);
      double fakeCenter[3]={-1,-1,-1};
      inInfo->Set(GDAMetaDataKeys::DIPOLE_CENTER(),fakeCenter,3);
      inInfo->Set(GDAMetaDataKeys::DIPOLE_CENTER(),this->Center,3);
      inInfo->Modified();
      this->Modified();
      //inInfo->Modified((vtkInformationKey*)GDAMetaDataKeys::DIPOLE_CENTER());
      cerr << "Found DIPOLE_CENTER." << endl;
      }

//     if (inInfo->Has(GDAMetaDataKeys::CELL_SIZE_RE()))
//       {
//       double cellSizeRe;
// 
//       inInfo->Get(GDAMetaDataKeys::CELL_SIZE_RE(),cellSizeRe);
//       this->Radius=2.5/cellSizeRe; // sphere will have 2.5 re.
// 
//       cerr << "Found CELL_SIZE_RE." << endl;
//       }

//   vtkInformation *outInfo = outInfos->GetInformationObject(0);
//   outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);
//   outInfo->Set(
//       vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),
//       this->Center[0]-this->Radius,
//       this->Center[0]+this->Radius,
//       this->Center[1]-this->Radius,
//       this->Center[1]+this->Radius,
//       this->Center[2]-this->Radius,
//       this->Center[2]+this->Radius);

  return 1;
}

//----------------------------------------------------------------------------
int vtkSQHemicylinderSource::RequestData(
      vtkInformation * /*req*/,
      vtkInformationVector ** /*inInfos*/,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQHemicylinderSourceDEBUG
  cerr << "===============================vtkSQHemicylinderSource::RequestData" << endl;
  #endif

  // generate a northern Hemicylinder
  vtkInformation *northInfo=outInfos->GetInformationObject(0);
  //
  if (this->NorthHemicylinderName && strlen(this->NorthHemicylinderName))
    {
    northInfo->Set(vtkSQMetaDataKeys::DESCRIPTIVE_NAME(),this->NorthHemicylinderName);
    }
  //
  vtkPolyData *northPd
    = vtkPolyData::SafeDownCast(northInfo->Get(vtkDataObject::DATA_OBJECT()));
  //
  GenerateHemicylinder(northPd,this->Center,this->Radius,this->Length,this->Resolution,0.0);


  // generate a southern Hemicylinder
  vtkInformation *southInfo=outInfos->GetInformationObject(1);
  //
  if (this->SouthHemicylinderName && strlen(this->SouthHemicylinderName))
    {
    southInfo->Set(vtkSQMetaDataKeys::DESCRIPTIVE_NAME(),this->SouthHemicylinderName);
    }
  //
  vtkPolyData *southPd
    = vtkPolyData::SafeDownCast(southInfo->Get(vtkDataObject::DATA_OBJECT()));
  //
  GenerateHemicylinder(southPd,this->Center,this->Radius,this->Length,this->Resolution,M_PI);

  return 1;
}

//----------------------------------------------------------------------------
void vtkSQHemicylinderSource::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef vtkSQHemicylinderSourceDEBUG
  cerr << "===============================vtkSQHemicylinderSource::PrintSelf" << endl;
  #endif
  // this->Superclass::PrintSelf(os,indent);

  os << indent << "Center "
     << this->Center[0] << ", "
     << this->Center[1] << ", "
     << this->Center[2] << endl
     << indent << "Radius "
     << this->Radius << endl
     << indent << "Resolution"
     << this->Resolution << endl
     << indent << "NorthHemicylinderName "
     << this->NorthHemicylinderName << endl
     << indent << "SouthHemicylinderName "
     << this->SouthHemicylinderName << endl;

  // TODO
}
