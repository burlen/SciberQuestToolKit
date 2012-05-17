/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#include "vtkSC11DemoReader.h"

#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMultiProcessController.h"
#include "vtkMPIController.h"

#include "vtkSC11DemoExtentTranslator.h"
#include "SC11DemoMetaData.h"
#include "SC11DemoReader.h"
#include "Tuple.hxx"
#include "PrintUtils.h"
#include "SQMacros.h"
#include "minmax.h"
#include "postream.h"

#include <mpi.h>

#include <sstream>
using std::ostringstream;

//#define vtkSC11DemoReaderDEBUG
#define vtkSC11DemoReaderTIME

#if defined vtkSC11DemoReaderTIME
  #include <sys/time.h>
  #include <unistd.h>
#endif

// disbale warning about passing string literals.
#if !defined(__INTEL_COMPILER) && defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif


vtkCxxRevisionMacro(vtkSC11DemoReader, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkSC11DemoReader);

//-----------------------------------------------------------------------------
vtkSC11DemoReader::vtkSC11DemoReader()
{
  #if defined vtkSC11DemoReaderDEBUG
  pCerr() << "===============================vtkSC11DemoReader" << endl;
  #endif

  // Initialize variables
  this->FileName=0;
  this->MetaData=new SC11DemoMetaData;
  this->Reader=new SC11DemoReader;
  this->DirtyValue=-1;

  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    vtkErrorMacro("MPI has not been initialized. Restart ParaView using mpiexec.");
    }

  // Initialize pipeline.
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkSC11DemoReader::~vtkSC11DemoReader()
{
  #if defined vtkSC11DemoReaderDEBUG
  pCerr() << "===============================~vtkSC11DemoReader" << endl;
  #endif

  delete this->MetaData;
  delete this->Reader;
}

//-----------------------------------------------------------------------------
int vtkSC11DemoReader::CanReadFile(const char *file)
{
  #if defined vtkSC11DemoReaderDEBUG
  pCerr() << "===============================CanReadFile" << endl;
  pCerr() << "Check " << safeio(file) << "." << endl;
  #endif

  string fileName(file);
  size_t extAt=fileName.rfind(".sc11");
  if (extAt==string::npos)
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

//-----------------------------------------------------------------------------
void vtkSC11DemoReader::SetFileName(const char* _arg)
{
  #if defined vtkSC11DemoReaderDEBUG
  pCerr()
    << "===============================SetFileName" << endl
    << safeio(_arg) << endl;
  #endif
  #if defined vtkSC11DemoReaderTIME
  double walls=0.0;
  timeval wallt;
  gettimeofday(&wallt,0x0);
  walls=(double)wallt.tv_sec+((double)wallt.tv_usec)/1.0E6;
  #endif

  vtkDebugMacro(<< this->GetClassName() << ": setting FileName to " << (_arg?_arg:"(null)"));
  if (this->FileName == NULL && _arg == NULL) { return;}
  if (this->FileName && _arg && (!strcmp(this->FileName,_arg))) { return;}
  if (this->FileName) { delete [] this->FileName; }
  if (_arg)
    {
    size_t n = strlen(_arg) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (_arg);
    this->FileName = cp1;
    do { *cp1++ = *cp2++; } while ( --n );
    }
  else
    {
    this->FileName = NULL;
    }
  this->Modified();

  SC11DemoMetaData *md=this->MetaData;
  int iErr;
  iErr=md->Open(this->FileName);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"Failed to open " <<  this->FileName);
    md->Print(cerr);
    return;
    }
  md->Print(cerr);

  iErr=this->Reader->Open(
        md->GetPathToBricks(),
        md->GetBrickExtension(),
        md->GetScalarName(),
        md->GetFileExtent());
  if (iErr)
    {
    sqErrorMacro(pCerr(),"Failed to map brick.");
    md->Print(cerr);
    this->Reader->Print(cerr);
    }
  this->Reader->Print(cerr);

  #if defined vtkSC11DemoReaderTIME
  gettimeofday(&wallt,0x0);
  double walle=(double)wallt.tv_sec+((double)wallt.tv_usec)/1.0E6;
  pCerr() << "vtkSC11DemoReader::SetFileName " << walle-walls << endl;
  #endif
}

//-----------------------------------------------------------------------------
int vtkSC11DemoReader::GetNumberOfTimeSteps()
{
  return this->MetaData->GetNumberOfTimeSteps();
}

//-----------------------------------------------------------------------------
void vtkSC11DemoReader::GetTimeSteps(double *times)
{
  int n=this->MetaData->GetNumberOfTimeSteps();

  for (int i=0; i<n; ++i)
    {
    times[i]=this->MetaData->GetTimeForStep(i);
    }
}

//----------------------------------------------------------------------------
int vtkSC11DemoReader::RequestDataObject(
      vtkInformation* /*req*/,
      vtkInformationVector** /*inInfos*/,
      vtkInformationVector* outInfos)
{
  #if defined vtkSC11DemoReaderDEBUG
  pCerr() << "===============================RequestDataObject" << endl;
  #endif

  vtkInformation* info=outInfos->GetInformationObject(0);

  vtkDataObject *dataset=vtkImageData::New();

  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkImageData");
  info->Set(vtkDataObject::DATA_EXTENT_TYPE(),VTK_3D_EXTENT);
  info->Set(vtkDataObject::DATA_OBJECT(),dataset);

  dataset->SetPipelineInformation(info);
  dataset->Delete();

  #if defined vtkSC11DemoReaderDEBUG
  pCerr() << "datasetType=" << info->Get(vtkDataObject::DATA_TYPE_NAME()) << endl;
  pCerr() << "dataset=" << info->Get(vtkDataObject::DATA_OBJECT()) << endl;
  pCerr() << "info="; info->Print(cerr);
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSC11DemoReader::RequestInformation(
  vtkInformation *req,
  vtkInformationVector**,
  vtkInformationVector* outInfos)
{
  #if defined vtkSC11DemoReaderDEBUG
  pCerr() << "===============================RequestInformationImage" << endl;
  #endif

  SC11DemoMetaData *md=this->MetaData;
  SC11DemoReader *r=this->Reader;

  vtkInformation *info=outInfos->GetInformationObject(0);

  // domain extent
  CartesianExtent domain(md->GetMemoryExtent());
  domain.CellToNode();

  info->Set(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    domain.GetData(),
    6);

  double X0[3]={0.0,0.0,0.0};
  info->Set(vtkDataObject::ORIGIN(),X0,3);

  double dX[3]={1.0,1.0,1.0};
  info->Set(vtkDataObject::SPACING(),dX,3);

  // time
  int nSteps=md->GetNumberOfTimeSteps();
  vector<double> times(nSteps,0.0);
  for (int i=0; i<nSteps; ++i)
    {
    times[i]=(double)md->GetTimeForStep(i);
    }

  info->Set(
    vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
    &times[0],
    times.size());

  double timeRange[2]={times[0],times[times.size()-1]};
  info->Set(
    vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
    timeRange,
    2);

  // specialized translator for our decomp
  CartesianExtent subset(r->GetMemoryExtent());
  subset.CellToNode();

  typedef vtkStreamingDemandDrivenPipeline vtkSDDPType;
  vtkSDDPType* sddp=dynamic_cast<vtkSDDPType*>(this->GetExecutive());

  vtkSC11DemoExtentTranslator *et
    = dynamic_cast<vtkSC11DemoExtentTranslator*>(sddp->GetExtentTranslator(info));
  if (!et)
    {
    et=vtkSC11DemoExtentTranslator::New();
    et->SetWholeExtent(domain);
    et->SetUpdateExtent(subset);
    sddp->SetExtentTranslator(info,et);
    et->Delete();
    }

  #if defined vtkSC11DemoReaderDEBUG
  pCerr()
    << "WHOLE_EXTENT=" << domain << endl
    << "UPDATE_EXTENT=" << subset << endl
    << "ORIGIN=" << Tuple<double>(X0,3) << endl
    << "SPACING=" << Tuple<double>(dX,3) << endl
    << "TIMES=" << times << endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSC11DemoReader::RequestData(
        vtkInformation *req,
        vtkInformationVector ** /*input*/,
        vtkInformationVector *outInfos)
{
  #if defined vtkSC11DemoReaderDEBUG
  pCerr() << "===============================RequestData" << endl;
  #endif
  #if defined vtkSC11DemoReaderTIME
  timeval wallt;
  double walls=0.0;
  gettimeofday(&wallt,0x0);
  walls=(double)wallt.tv_sec+((double)wallt.tv_usec)/1.0E6;
  #endif

  vtkInformation *info=outInfos->GetInformationObject(0);

  // Get the output dataset.
  vtkDataSet *output
    = dynamic_cast<vtkDataSet *>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (output==NULL)
    {
    vtkErrorMacro("Filter data has not been configured correctly.");
    return 1;
    }

  SC11DemoMetaData *md=this->MetaData;
  SC11DemoReader *r=this->Reader;

  // fake the time
  if (info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    double *step
      = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());

    info->Set(
      vtkDataObject::DATA_TIME_STEPS(),
      step,
      1);

    output->GetInformation()->Set(
      vtkDataObject::DATA_TIME_STEPS(),
      step,
      1);
    }

  // Configure the output.
  // get the requested update extent, change it to what we
  // really have.
  CartesianExtent reqDecomp;
  info->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    reqDecomp.GetData());

  CartesianExtent decomp(r->GetMemoryExtent());
  decomp.CellToNode();
  info->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    decomp.GetData(),
    6);

  double dX[3];
  info->Get(vtkDataObject::SPACING(),dX);

  double X0[3];
  info->Get(vtkDataObject::ORIGIN(),X0);

  int nPoints[3];
  decomp.Size(nPoints);

  vtkImageData *idds=dynamic_cast<vtkImageData*>(output);
  idds->SetOrigin(X0);
  idds->SetSpacing(dX);
  idds->SetExtent(decomp.GetData());

  // pass in the array
  vtkFloatArray *scalar=vtkFloatArray::New();
  scalar->SetName(md->GetScalarName());

  r->CopyTo(scalar);

  idds->GetPointData()->AddArray(scalar);
  scalar->Delete();

  #if defined vtkSC11DemoReaderDEBUG
  idds->Print(cerr);

  pCerr()
    << "UPDATE_EXTENT(requested)=" << reqDecomp << endl
    << "UPDATE_EXTENT(actual)=" << decomp << endl;
  #endif

  #if defined vtkSC11DemoReaderTIME
  gettimeofday(&wallt,0x0);
  double walle=(double)wallt.tv_sec+((double)wallt.tv_usec)/1.0E6;
  pCerr() << "vtkSC11DemoReader::RequestData " << walle-walls << endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSC11DemoReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << endl;
}

