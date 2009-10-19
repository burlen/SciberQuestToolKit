/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "vtkOOCBOVReader.h"

#include "vtkObjectFactory.h"
#include "vtkAMRBox.h"
#include "vtkImageData.h"

#include "BOVMetaData.h"
#include "BOVReader.h"
#include "BOVTimeStepImage.h"

vtkCxxRevisionMacro(vtkOOCBOVReader, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkOOCBOVReader);

//-----------------------------------------------------------------------------
vtkOOCBOVReader::vtkOOCBOVReader()
    :
  Reader(0),
  Image(0)
{
  this->Reader=new BOVReader;
}

//-----------------------------------------------------------------------------
vtkOOCBOVReader::~vtkOOCBOVReader()
{
  delete this->Reader;
  if (this->Image)
    {
    this->Close();
    }
}

//-----------------------------------------------------------------------------
void vtkOOCBOVReader::SetReader(BOVReader *reader)
{
  delete this->Reader;
  this->Reader=new BOVReader;
  *this->Reader=*reader;
  // Force solo reads!
  this->Reader->SetCommunicator(MPI_COMM_SELF);
}

//-----------------------------------------------------------------------------
int vtkOOCBOVReader::Open()
{
  if (this->Image)
    {
    this->Close();
    }
  this->Image=this->Reader->OpenTimeStep(this->TimeIndex);
  if (!this->Image)
    {
    vtkWarningMacro("Failed to open file image!");
    return 0;
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkOOCBOVReader::Close()
{
  this->Reader->CloseTimeStep(this->Image);
  this->Image=0;
}

//-----------------------------------------------------------------------------
vtkDataSet *vtkOOCBOVReader::Read(double b[6])
{
  vtkWarningMacro("Not implemented!");
  return 0;
}

//-----------------------------------------------------------------------------
vtkDataSet *vtkOOCBOVReader::ReadNeighborhood(double p[3], int size)
{
  // Locate the cell where this point lies.
  double X0[3];
  double dX[3];

  const vtkAMRBox &domain=this->Reader->GetMetaData()->GetDomain();
  domain.GetDataSetOrigin(X0);
  domain.GetGridSpacing(dX);

  int cellId[3];
  cellId[0]=static_cast<int>((p[0]-X0[0])/dX[0]);
  cellId[1]=static_cast<int>((p[1]-X0[1])/dX[1]);
  cellId[2]=static_cast<int>((p[2]-X0[2])/dX[2]);

  // Make a box that contains it
  vtkAMRBox decomp(cellId[0],cellId[1],cellId[2],cellId[0],cellId[1],cellId[2]);
  decomp.SetDataSetOrigin(X0);
  decomp.SetGridSpacing(dX);
  decomp.Grow(size/2);
  decomp&=domain;

  #if defined vtkOOCBOVReaderDEBUG
  static int ww=0;
  ++ww;
  cerr << ww
       << " Reading ("
       << p[0] << ", " << p[1] << ", " << p[2] << ") -> ";
  cerr << decomp.Print(cerr) << endl;
  #endif

  // Set up a vtk dataset to hold the results.
  int subset[6];
  int nPoints[3];
  decomp.GetDimensions(subset);
  decomp.GetNumberOfCells(nPoints); // dual grid
  decomp.GetBoxOrigin(X0);

  vtkImageData *idds=vtkImageData::New();
  idds->SetDimensions(nPoints);
  idds->SetOrigin(X0);
  idds->SetSpacing(dX);

  // Actual read.
  this->Reader->GetMetaData()->SetDecomp(decomp);
  int ok=this->Reader->ReadTimeStep(this->Image,idds,(vtkAlgorithm*)0);
  if (!ok)
    {
    vtkErrorMacro("Read failed. Aborting.");
    return 0;
    }
  return idds;
}

//-----------------------------------------------------------------------------
void vtkOOCBOVReader::ActivateArray(const char *name)
{
  this->Reader->GetMetaData()->ActivateArray(name);
}

//-----------------------------------------------------------------------------
void vtkOOCBOVReader::DeActivateArray(const char *name)
{
  this->Reader->GetMetaData()->DeactivateArray(name);
}

//-----------------------------------------------------------------------------
void vtkOOCBOVReader::DeActivateAllArrays()
{
  int nArray=this->Reader->GetMetaData()->GetNumberOfArrays();
  for (int i=0; i<nArray; ++i)
    {
    const char *name=this->Reader->GetMetaData()->GetArrayName(i);
    this->Reader->GetMetaData()->DeactivateArray(name);
    }
}

//-----------------------------------------------------------------------------
void vtkOOCBOVReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent.GetNextIndent());
  os << indent << "Reader: " << endl;
  this->Reader->Print(os);
  os << endl;
}
