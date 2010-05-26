/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/ 
#include "vtkSQBOVReader.h"

#include "vtkObjectFactory.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkImageData.h"
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

#include "vtkSQOOCReader.h"
#include "vtkSQOOCBOVReader.h"
#include "BOVReader.h"
#include "GDAMetaData.h"
#include "BOVTimeStepImage.h"
#include "CartesianDecomp.h"
#include "Tuple.hxx"
#include "PrintUtils.h"
#include "SQMacros.h"
#include "minmax.h"

#include <mpi.h>

#include <sstream>
using std::ostringstream;

// for gethostname on windows.
#ifdef WIN32
  #include <Winsock2.h>
#endif

#ifndef HOST_NAME_MAX
  #define HOST_NAME_MAX 255
#endif

// #define vtkSQBOVReaderDEBUG

#if defined vtkSQBOVReaderTIME
  #include <sys/time.h>
  #include <unistd.h>
#endif

// disbale warning about passing string literals.
#pragma GCC diagnostic ignored "-Wwrite-strings"

vtkCxxRevisionMacro(vtkSQBOVReader, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkSQBOVReader);

// Compare two doubles.
int fequal(double a, double b, double tol)
{
  double pda=fabs(a);
  double pdb=fabs(b);
  pda=pda<tol?tol:pda;
  pdb=pdb<tol?tol:pdb;
  double smaller=pda<pdb?pda:pdb;
  double norm=fabs(b-a)/smaller;
  if (norm<=tol)
    {
    return 1;
    }
  return 0;
}


//-----------------------------------------------------------------------------
vtkSQBOVReader::vtkSQBOVReader()
{
  #if defined vtkSQBOVReaderDEBUG
  cerr << "===============================vtkSQBOVReader" << endl;
  #endif

  // Initialize variables
  this->MetaRead=0;
  this->FileName=0;
  this->FileNameChanged=false;
  this->Subset[0]=
  this->Subset[1]=
  this->Subset[2]=
  this->Subset[3]=
  this->Subset[4]=
  this->Subset[5]=0;
  this->ISubsetRange[0]=1;
  this->ISubsetRange[1]=0;
  this->JSubsetRange[0]=1;
  this->JSubsetRange[1]=0;
  this->KSubsetRange[0]=1;
  this->KSubsetRange[1]=0;
  this->PeriodicBC[0]=
  this->PeriodicBC[1]=
  this->PeriodicBC[2]=0;
  this->DecompDims[0]=
  this->DecompDims[1]=
  this->DecompDims[2]=1;
  this->BlockCacheSize=10;
  this->ClearCachedBlocks=1;
  this->UseCollectiveIO=HINT_ENABLED;
  this->NumberOfIONodes=0;
  this->CollectBufferSize=0;
  this->UseDataSieving=HINT_AUTOMATIC;
  this->SieveBufferSize=0;
  this->WorldRank=0;
  this->WorldSize=1;

  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    vtkErrorMacro("MPI has not been initialized. Restart ParaView using mpiexec.");
    }

  vtkMultiProcessController *con=vtkMultiProcessController::GetGlobalController();
  this->WorldRank=con->GetLocalProcessId();
  this->WorldSize=con->GetNumberOfProcesses();

  this->HostName[0]='\0';
  #if defined vtkSQBOVReaderDEBUG
  char hostname[HOST_NAME_MAX];
  gethostname(hostname,HOST_NAME_MAX);
  hostname[4]='\0';
  for (int i=0; i<5; ++i)
    {
    this->HostName[i]=hostname[i];
    }
  #endif

  // Configure the internal reader.
  this->Reader=BOVReader::New();

  GDAMetaData md;
  this->Reader->SetMetaData(&md);

  // Initialize pipeline.
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkSQBOVReader::~vtkSQBOVReader()
{
  #if defined vtkSQBOVReaderDEBUG
  cerr << "===============================~vtkSQBOVReader" << endl;
  #endif

  this->Clear();
  this->Reader->Delete();
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::Clear()
{
  this->SetFileName(0);
  this->FileNameChanged=false;
  this->Subset[0]=
  this->Subset[1]=
  this->Subset[2]=
  this->Subset[3]=
  this->Subset[4]=
  this->Subset[5]=0;
  this->ISubsetRange[0]=1;
  this->ISubsetRange[1]=0;
  this->JSubsetRange[0]=1;
  this->JSubsetRange[1]=0;
  this->KSubsetRange[0]=1;
  this->KSubsetRange[1]=0;
  this->UseCollectiveIO=HINT_ENABLED;
  this->NumberOfIONodes=0;
  this->CollectBufferSize=0;
  this->UseDataSieving=HINT_AUTOMATIC;
  this->SieveBufferSize=0;
  this->Reader->Close();
}

//-----------------------------------------------------------------------------
int vtkSQBOVReader::CanReadFile(const char *file)
{
  #if defined vtkSQBOVReaderDEBUG
  cerr << "===============================CanReadFile" << endl;
  cerr << "Check " << safeio(file) << "." << endl;
  #endif

  this->Reader->SetCommunicator(MPI_COMM_SELF);
  int status=this->Reader->Open(file);
  this->Reader->Close();

  return status;
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetFileName(const char* _arg)
{
  #if defined vtkSQBOVReaderDEBUG
  cerr << "===============================SetFileName" << endl;
  cerr << "Set FileName from " << safeio(this->FileName) << " to " << safeio(_arg) << "." << endl;
  #endif
  #if defined vtkSQBOVReaderTIME
  double walls=0.0;
  timeval wallt;
  if (this->WorldRank==0)
    {
    gettimeofday(&wallt,0x0);
    walls=(double)wallt.tv_sec+((double)wallt.tv_usec)/1.0E6;
    }
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

  // Close the currently opened dataset (if any).
  if (this->Reader->IsOpen())
    {
    this->Reader->Close();
    }

  // Open the newly named dataset.
  if (this->FileName)
    {
    this->Reader->SetCommunicator(MPI_COMM_WORLD);
    if(!this->Reader->Open(this->FileName))
      {
      vtkErrorMacro("Failed to open the file \"" << safeio(this->FileName) << "\".");
      return;
      }

    // Update index space ranges provided in the U.I.
    const CartesianExtent &subset=this->Reader->GetMetaData()->GetSubset();
    this->ISubsetRange[0]=this->Subset[0]=subset[0];
    this->ISubsetRange[1]=this->Subset[1]=subset[1];
    this->JSubsetRange[0]=this->Subset[2]=subset[2];
    this->JSubsetRange[1]=this->Subset[3]=subset[3];
    this->KSubsetRange[0]=this->Subset[4]=subset[4];
    this->KSubsetRange[1]=this->Subset[5]=subset[5];

    #if defined vtkSQBOVReaderDEBUG
    cerr
      << "vtkSQBOVReader "
      << this->HostName << " "
      << this->WorldRank
      << " Open succeeded." << endl;
    #endif
    }

  this->Modified();

  #if defined vtkSQBOVReaderTIME
  MPI_Barrier(MPI_COMM_WORLD);
  if (this->WorldRank==0)
    {
    gettimeofday(&wallt,0x0);
    double walle=(double)wallt.tv_sec+((double)wallt.tv_usec)/1.0E6;
    cerr << "vtkSQBOVReader::SetFileName " << walle-walls << endl;
    }
  #endif
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetSubset(const int *s)
{
  #if defined vtkSQBOVReaderDEBUG
  cerr << "===============================SetSubset*" << endl;
  #endif
  this->SetSubset(s[0],s[1],s[2],s[3],s[4],s[5]);
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetSubset(int ilo,int ihi, int jlo, int jhi, int klo, int khi)
{
  #if defined vtkSQBOVReaderDEBUG
  cerr << "===============================SetSubset" << endl;
  #endif
  // Avoid unecessary pipeline execution.
  if ( this->Subset[0]==ilo && this->Subset[1]==ihi
    && this->Subset[2]==jlo && this->Subset[3]==jhi
    && this->Subset[4]==klo && this->Subset[5]==khi )
    {
    return;
    }

  // Copy for get pointer return.
  this->Subset[0]=ilo;
  this->Subset[1]=ihi;
  this->Subset[2]=jlo;
  this->Subset[3]=jhi;
  this->Subset[4]=klo;
  this->Subset[5]=khi;

  // Pass through to reader.
  CartesianExtent subset(this->Subset);
  this->Reader->GetMetaData()->SetSubset(subset);

  // Mark object dirty.
  this->Modified();

  #if defined vtkSQBOVReaderDEBUG
  cerr << "SetSubset(" << subset << ")" << endl;
  #endif
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetISubset(int ilo, int ihi)
{
  int *s=this->Subset;
  this->SetSubset(ilo,ihi,s[2],s[3],s[4],s[5]);
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetJSubset(int jlo, int jhi)
{
  int *s=this->Subset;
  this->SetSubset(s[0],s[1],jlo,jhi,s[4],s[5]);
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetKSubset(int klo, int khi)
{
  int *s=this->Subset;
  this->SetSubset(s[0],s[1],s[2],s[3],klo,khi);
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetPeriodicBC(int *flags)
{
  if ( (this->PeriodicBC[0]==flags[0])
    && (this->PeriodicBC[1]==flags[1])
    && (this->PeriodicBC[2]==flags[2]) )
    {
    return;
    }

  this->PeriodicBC[0]=flags[0];
  this->PeriodicBC[1]=flags[1];
  this->PeriodicBC[2]=flags[2];

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetXHasPeriodicBC(int flag) 
{
  if (this->PeriodicBC[0]==flag)
    {
    return;
    }

  this->PeriodicBC[0]=flag;

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetYHasPeriodicBC(int flag)
{
  if (this->PeriodicBC[1]==flag)
    {
    return;
    }

  this->PeriodicBC[1]=flag;

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetZHasPeriodicBC(int flag)
{
  if (this->PeriodicBC[2]==flag)
    {
    return;
    }

  this->PeriodicBC[2]=flag;

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetPointArrayStatus(const char *name, int status)
{
  #if defined vtkSQBOVReaderDEBUG
  cerr << "===============================SetPointArrayStatus" << endl;
  cerr << safeio(name) << " " << status << endl;
  #endif
  if (status)
    {
    this->Reader->GetMetaData()->ActivateArray(name);
    }
  else
    {
    this->Reader->GetMetaData()->DeactivateArray(name);
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQBOVReader::GetPointArrayStatus(const char *name)
{
  #if defined vtkSQBOVReaderDEBUG
  cerr << "===============================GetPointArrayStatus" << endl;
  #endif
  return this->Reader->GetMetaData()->IsArrayActive(name);
}

//-----------------------------------------------------------------------------
int vtkSQBOVReader::GetNumberOfPointArrays()
{
  #if defined vtkSQBOVReaderDEBUG
  cerr << "===============================GetNumberOfPointArrays" << endl;
  #endif
  return this->Reader->GetMetaData()->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkSQBOVReader::GetPointArrayName(int idx)
{
  #if defined vtkSQBOVReaderDEBUG
  cerr << "===============================GetArrayName" << endl;
  #endif
  return this->Reader->GetMetaData()->GetArrayName(idx);
}

//-----------------------------------------------------------------------------
int vtkSQBOVReader::RequestInformation(
  vtkInformation* req,
  vtkInformationVector**,
  vtkInformationVector* outInfos)
{
  #if defined vtkSQBOVReaderDEBUG
  cerr << "===============================RequestInformation" << endl;
  #endif

  if (!this->Reader->IsOpen())
    {
    vtkWarningMacro("No file open, cannot process RequestInformation!");
    return 1;
    }

  vtkInformation *info=outInfos->GetInformationObject(0);

  double X0[3]={0.0,0.0,0.0};
  double dX[3]={1.0,1.0,1.0};
  int wholeExtent[6]={0,1,0,1,0,1};

  // The two modes to run the reader are Meta mode and Actual mode.
  // In meta mode no data is read rather the pipeline is tricked into
  // ploting a bounding box matching the data size. Keys are provided
  // so that actual read may take place downstream. In actual mode
  // the reader reads the requested arrays.
  if (this->MetaRead)
    {
    // In a meta read we need to trick the pipeline into
    // displaying the dataset bounds, in a single cell per
    // process. This keeps the memory footprint minimal.
    // We still need to provide the actual meta data for
    // use downstream, including a file name so that filters
    // may use it to read the actual data, we do so using
    // keys provided having the same name as the original.

    // Set the extent of the data.
    // The point extent given PV by the meta reader will always start from
    // 0 and range to nProcs and 1. This will neccesitate a false origin
    // and spacing as well.
    wholeExtent[1]=this->WorldSize;

    // Save the real extent in our own key.
    // info->Set(vtkSQOOCReader::WHOLE_EXTENT(),this->Subset,6);
    // Set the origin and spacing
    // We save the actuals in our own keys.
    // info->Set(vtkSQOOCReader::ORIGIN(),X0,3);
    // info->Set(vtkSQOOCReader::SPACING(),dX,3);

    // Adjust PV's keys for the false subsetting extents.
    X0[0]=X0[0]+this->Subset[0]*dX[0];
    X0[1]=X0[1]+this->Subset[2]*dX[1];
    X0[2]=X0[2]+this->Subset[4]*dX[2];

    // Adjust grid spacing for our single cell per process. We are using the dual
    // grid so we are subtracting 1 to get the number of cells.
    int nCells[3]={
      this->Subset[1]-this->Subset[0],
      this->Subset[3]-this->Subset[2],
      this->Subset[5]-this->Subset[4]};
    dX[0]=dX[0]*((double)nCells[0])/((double)this->WorldSize);
    dX[1]=dX[1]*((double)nCells[1]);
    dX[2]=dX[2]*((double)nCells[2]);

    // Set the file name so that filter who process the data may read
    // as neccessary.
    // info->Set(vtkSQOOCReader::FILE_NAME(),this->FileName);
    }
  else
    {
    // The actual read we need to populate the VTK keys with actual
    // values. The mechanics of the pipeline require that the data set
    // dimensions and whole extent key reflect the global index space
    // of the dataset, the data set extent will have the decomposed 
    // index space.
    this->GetSubset(wholeExtent);
    //
    this->Reader->GetMetaData()->GetOrigin(X0);
    this->Reader->GetMetaData()->GetSpacing(dX);
    }

  // Pass values into the pipeline.
  info->Set(vtkDataObject::ORIGIN(),X0,3);
  info->Set(vtkDataObject::SPACING(),dX,3);
  info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExtent,6);

  // Determine which time steps are available.
  int nSteps=this->Reader->GetMetaData()->GetNumberOfTimeSteps();
  const int *steps=this->Reader->GetMetaData()->GetTimeSteps();
  vector<double> times(nSteps,0.0);
  for (int i=0; i<nSteps; ++i)
    {
    times[i]=(double)steps[i]; // use the index rather than the actual.
    }

  #if defined vtkSQBOVReaderDEBUG
  cerr << times << endl;
  cerr << "Total: " << nSteps << endl;
  #endif

  // Set available time steps on pipeline.
  info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),&times[0],times.size());
  double timeRange[2]={times[0],times[times.size()-1]};
  info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),timeRange,2);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetMPIFileHints()
{
  MPI_Info hints;
  MPI_Info_create(&hints);

  switch (this->UseCollectiveIO)
    {
    case HINT_AUTOMATIC:
      // do nothing, it's up to implementation.
      break;
    case HINT_DISABLED:
      MPI_Info_set(hints,"romio_cb_read","disable");
      break;
    case HINT_ENABLED:
      MPI_Info_set(hints,"romio_cb_read","enable");
      MPI_Info_set(hints,"romio_no_indep_rw","true");
      break;
    default:
      vtkErrorMacro("Invalid value for UseCollectiveIO.");
      break;
    }

  if (this->NumberOfIONodes>0)
    {
    ostringstream os;
    os << this->NumberOfIONodes;
    MPI_Info_set(hints,"cb_nodes",const_cast<char *>(os.str().c_str()));
    }

  if (this->CollectBufferSize>0)
    {
    ostringstream os;
    os << this->CollectBufferSize;
    MPI_Info_set(hints,"cb_buffer_size",const_cast<char *>(os.str().c_str()));
    }

  switch (this->UseDataSieving)
    {
    case HINT_AUTOMATIC:
      // do nothing, it's up to implementation.
      break;
    case HINT_DISABLED:
      MPI_Info_set(hints,"romio_ds_read","disable");
      break;
    case HINT_ENABLED:
      MPI_Info_set(hints,"romio_ds_read","enable");
      break;
    default:
      vtkErrorMacro("Invalid value for UseDataSieving.");
      break;
    }

  if (this->SieveBufferSize>0)
    {
    ostringstream os;
    os << this->SieveBufferSize;
    MPI_Info_set(hints,"ind_rd_buffer_size", const_cast<char *>(os.str().c_str()));
    }

  this->Reader->SetHints(hints);

  MPI_Info_free(&hints);
}

//-----------------------------------------------------------------------------
int vtkSQBOVReader::RequestData(
        vtkInformation * /*req*/,
        vtkInformationVector ** /*input*/,
        vtkInformationVector *outInfos)
{
  #if defined vtkSQBOVReaderDEBUG
  cerr << "===============================RequestData" << endl;
  #endif
  #if defined vtkSQBOVReaderTIME
  double walls=0.0;
  timeval wallt;
  if (this->WorldRank==0)
    {
    gettimeofday(&wallt,0x0);
    walls=(double)wallt.tv_sec+((double)wallt.tv_usec)/1.0E6;
    }
  #endif

  vtkInformation *info=outInfos->GetInformationObject(0);

  // Get the output dataset.
  vtkImageData *idds
    = dynamic_cast<vtkImageData *>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (idds==NULL)
    {
    vtkErrorMacro("Filter data has not been configured correctly.");
    return 1;
    }

  // Figure out which of our time steps is closest to the requested
  // one, fallback to the 0th step.
  int stepId=this->Reader->GetMetaData()->GetTimeStep(0);
  if (info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    double *step
      = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    int nSteps
      = info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double* steps =
      info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    for (int i=0; i<nSteps; ++i)
      {
      if (fequal(steps[i],*step,1E-10))
        {
        stepId=this->Reader->GetMetaData()->GetTimeStep(i);
        break;
        }
      }
    info->Set(vtkDataObject::DATA_TIME_STEPS(),step,1);
    #if defined vtkSQBOVReaderDEBUG
    cerr << "Requested time " << *step << " using " << stepId << "." << endl;
    #endif
    }

  // The two modes to run the reader are Meta mode and Actual mode.
  // In meta mode no data is read rather the pipeline is tricked into
  // ploting a bounding box matching the data size. Keys are provided
  // so that actual read may take place downstream. In actual mode
  // the reader reads the requested arrays.


  // index space selected in the UI.
  CartesianExtent subset=this->Reader->GetMetaData()->GetSubset();

  // Set the number of points in the output depending on the
  // reader mode.
  int nPoints[3]={2,2,2};
  if (this->MetaRead)
    {
    // A meta read makes a grid consisting of nProcs cells along
    // the x-axis minimizing the memory footprint while allowing domain
    // decomposition to take place.
    nPoints[0]=this->WorldSize+1;
    nPoints[1]=2;
    nPoints[2]=2;
    }
  else
    {
    // A read pulls data from file on a cell centered grid
    // into memory on a node centered grid. The memory grid
    // is the dual grid of the file grid. In other words
    // file->nCells==memory->nPoints
    subset.Size(nPoints);
    }

  // now that we have the number of points, convert the
  // subset extent from a node based extent to a cell based
  // one.
  subset.NodeToCell(); // dual grid

  // Get the values for origin and spacing from our previous computation
  // in RequestInformation. For a meta-read these are pseudo values.
  double dX[3];
  info->Get(vtkDataObject::SPACING(),dX);
  double X0[3];
  info->Get(vtkDataObject::ORIGIN(),X0);

  // The extent is expetcted to be set to what is actually in
  // memory on this process. Paraview tells us what this is.
  int decomp[6];
  info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),decomp);

  // Configure the grid in preparation for the read.
  idds->SetDimensions(nPoints);
  idds->SetOrigin(X0);
  idds->SetSpacing(dX);
  idds->SetExtent(decomp);

  #if defined vtkSQBOVReaderDEBUG
  cerr
    << "RequestData "
    << this->HostName << " "
    << this->WorldRank << " "
    << this->Reader->GetMetaData()->GetSubset().Size() << " "
    << Tuple<int>(decomp,6)
    << endl;
  #endif

  // Construct MPI File hints for the reader.
  this->SetMPIFileHints();

  // Read the step.
  int ok;
  if (this->MetaRead)
    {
    // Meta read.
    ok=this->Reader->ReadMetaTimeStep(stepId,idds,this);

    // Make a domain decomposition of the requested subset.
    double *subsetX0=this->Reader->GetMetaData()->GetOrigin();
    double *subsetDX=this->Reader->GetMetaData()->GetSpacing();

    CartesianExtent fileExt=this->Reader->GetMetaData()->GetDomain();
    fileExt.NodeToCell(); // dual grid

    CartesianDecomp *ddecomp=CartesianDecomp::New();
    ddecomp->SetFileExtent(fileExt);
    ddecomp->SetExtent(subset);
    ddecomp->SetOrigin(subsetX0);
    ddecomp->SetSpacing(subsetDX);
    ddecomp->ComputeBounds();
    ddecomp->SetDecompDims(this->DecompDims);
    ddecomp->SetPeriodicBC(this->PeriodicBC);
    ddecomp->SetNumberOfGhostCells(this->NGhosts);
    ddecomp->DecomposeDomain();

    // Pass the reader into the pipeline, actual read takes
    // place later intiated by downstream filter.
    vtkSQOOCBOVReader *OOCReader=vtkSQOOCBOVReader::New();
    OOCReader->SetReader(this->Reader);
    OOCReader->SetTimeIndex(stepId);
    OOCReader->SetDomainDecomp(ddecomp);
    OOCReader->SetBlockCacheSize(this->BlockCacheSize);
    OOCReader->SetCloseClearsCachedBlocks(this->ClearCachedBlocks);
    OOCReader->InitializeBlockCache();
    info->Set(vtkSQOOCReader::READER(),OOCReader);
    OOCReader->Delete();
    ddecomp->Delete();

    // Pass the bounds of what would have been read on all
    // processes. The subset describes what the user has
    // marked for reading.
    double subsetBounds[6];
    subsetBounds[0]=X0[0];
    subsetBounds[1]=X0[0]+dX[0]*((double)this->WorldSize);
    subsetBounds[2]=X0[1];
    subsetBounds[3]=X0[1]+dX[1];
    subsetBounds[4]=X0[2];
    subsetBounds[5]=X0[2]+dX[2];
    info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),subsetBounds,6);

    // pass the boundary condition flags
    info->Set(vtkSQOOCReader::PERIODIC_BC(),this->PeriodicBC,3);
    }
  else
    {
    // Read.
    this->Reader->GetMetaData()->SetDecomp(decomp);

    BOVTimeStepImage *stepImg=this->Reader->OpenTimeStep(stepId);

    ok=this->Reader->ReadTimeStep(stepImg,idds,this);

    this->Reader->CloseTimeStep(stepImg);

    // pass the bounds
    double subsetBounds[6];
    subset.GetBounds(X0,dX,subsetBounds);

    info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),subsetBounds,6);
    }
  if (!ok)
    {
    vtkErrorMacro(
      << "Read failed." << endl
      << *this->Reader->GetMetaData()
      );
    return 1;
    }

  // Give implementation classes a chance to push specialized
  // keys.
  this->Reader->GetMetaData()->PushPipelineInformation(info);

  #if defined vtkSQBOVReaderDEBUG
  this->Reader->PrintSelf(cerr);
  idds->Print(cerr);
  #endif

  #if defined vtkSQBOVReaderTIME
  MPI_Barrier(MPI_COMM_WORLD);
  if (this->WorldRank==0)
    {
    gettimeofday(&wallt,0x0);
    double walle=(double)wallt.tv_sec+((double)wallt.tv_usec)/1.0E6;
    cerr << "vtkSQBOVReader::RequestData " << walle-walls << endl;
    }
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName:        " << safeio(this->FileName) << endl;
  os << indent << "FileNameChanged: " << this->FileNameChanged << endl;
  os << indent << "Raeder: " << endl;
  this->Reader->PrintSelf(os);
  os << endl;
}

/// PV U.I.
// //----------------------------------------------------------------------------
// // observe PV interface and set modified if user makes changes
// void vtkSQBOVReader::SelectionModifiedCallback(
//         vtkObject*,
//         unsigned long,
//         void* clientdata,
//         void*)
// {
//   vtkSQBOVReader *dbb
//     = static_cast<vtkSQBOVReader*>(clientdata);
// 
//   dbb->Modified();
// }

