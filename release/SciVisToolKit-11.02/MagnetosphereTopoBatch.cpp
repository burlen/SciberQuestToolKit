/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "vtkMultiProcessController.h"
#include "vtkMPIController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkSmartPointer.h"

#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkXMLPDataSetWriter.h"

#include "SQMacros.h"
#include "postream.h"
#include "vtkSQBOVReader.h"
#include "vtkSQFieldTracer.h"
#include "vtkSQPlaneSource.h"
#include "vtkSQVolumeSource.h"
#include "vtkSQHemisphereSource.h"
#include "XMLUtils.h"

#include <sstream>
using std::ostringstream;
#include <iostream>
using std::cerr;
using std::endl;
#include <iomanip>
using std::setfill;
using std::setw;
#include <vector>
using std::vector;
#include <string>
using std::string;

#include <mpi.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define SQ_EXIT_ERROR 1
#define SQ_EXIT_SUCCESS 0

//*****************************************************************************
void PVTK_Exit(vtkMPIController* controller, int code)
{
  controller->Finalize();
  controller->Delete();
  vtkAlgorithm::SetDefaultExecutivePrototype(0);
  exit(code);
}

//*****************************************************************************
int IndexOf(double value, double *values, int first, int last)
{
  int mid=(first+last)/2;
  if (values[mid]==value)
    {
    return mid;
    }
  else
  if (mid!=first && values[mid]>value)
    {
    return IndexOf(value,values,first,mid-1);
    }
  else
  if (mid!=last && values[mid]<value)
    {
    return IndexOf(value,values,mid+1,last);
    }
  return -1;
}

//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  vtkSmartPointer<vtkMPIController> controller=vtkSmartPointer<vtkMPIController>::New();

  controller->Initialize(&argc,&argv,0);
  int worldRank=controller->GetLocalProcessId();
  int worldSize=controller->GetNumberOfProcesses();

  vtkMultiProcessController::SetGlobalController(controller);

  vtkCompositeDataPipeline* cexec=vtkCompositeDataPipeline::New();
  vtkAlgorithm::SetDefaultExecutivePrototype(cexec);
  cexec->Delete();

  if (argc<3)
    {
    if (worldRank==0)
      {
      pCerr()
        << "Error: Command tail." << endl
        << " 1) /path/to/runConfig.xml" << endl
        << " 2) /path/to/file.bovm" << endl
        << " 3) /path/to/output/" << endl
        << " 4) startTime" << endl
        << " 5) endTime" << endl
        << endl;
      }
    vtkAlgorithm::SetDefaultExecutivePrototype(0);
    return SQ_EXIT_ERROR;
    }

  // distribute the configuration file name and time range
  int configNameLen=0;
  char *configName=0;

  int bovFileNameLen=0;
  char *bovFileName=0;

  int outputPathLen=0;
  char *outputPath=0;

  double startTime=-1.0;
  double endTime=-1.0;

  if (worldRank==0)
    {
    configNameLen=strlen(argv[1])+1;
    configName=(char *)malloc(configNameLen);
    strncpy(configName,argv[1],configNameLen);
    controller->Broadcast(&configNameLen,1,0);
    controller->Broadcast(configName,configNameLen,0);

    bovFileNameLen=strlen(argv[2])+1;
    bovFileName=(char *)malloc(bovFileNameLen);
    strncpy(bovFileName,argv[2],bovFileNameLen);
    controller->Broadcast(&bovFileNameLen,1,0);
    controller->Broadcast(bovFileName,bovFileNameLen,0);

    outputPathLen=strlen(argv[3])+1;
    outputPath=(char *)malloc(outputPathLen);
    strncpy(outputPath,argv[3],outputPathLen);
    controller->Broadcast(&outputPathLen,1,0);
    controller->Broadcast(outputPath,outputPathLen,0);

    // times are optional if not provided entire series is
    // used.
    if (argc>4)
      {
      startTime=atof(argv[4]);
      endTime=atof(argv[5]);
      }
    controller->Broadcast(&startTime,1,0);
    controller->Broadcast(&endTime,1,0);
    }
  else
    {
    controller->Broadcast(&configNameLen,1,0);
    configName=(char *)malloc(configNameLen);
    controller->Broadcast(configName,configNameLen,0);

    controller->Broadcast(&bovFileNameLen,1,0);
    bovFileName=(char *)malloc(bovFileNameLen);
    controller->Broadcast(bovFileName,bovFileNameLen,0);

    controller->Broadcast(&outputPathLen,1,0);
    outputPath=(char *)malloc(outputPathLen);
    controller->Broadcast(outputPath,outputPathLen,0);

    controller->Broadcast(&startTime,1,0);
    controller->Broadcast(&endTime,1,0);
    }

  // read the configuration file.
  int iErr=0;
  vtkSmartPointer<vtkPVXMLParser> parser=vtkSmartPointer<vtkPVXMLParser>::New();
  parser->SetFileName(configName);
  if (parser->Parse()==0)
    {
    sqErrorMacro(pCerr(),"Invalid XML in file " << configName << ".");
    return SQ_EXIT_ERROR;
    }

  // check for the semblance of a valid configuration hierarchy
  vtkPVXMLElement *root=parser->GetRootElement();
  if (root==0)
    {
    sqErrorMacro(pCerr(),"Invalid XML in file " << configName << ".");
    return SQ_EXIT_ERROR;
    }

  string requiredType("MagnetosphereTopologyBatch");
  const char *foundType=root->GetName();
  if (foundType==0 || foundType!=requiredType)
    {
    sqErrorMacro(pCerr(),
        << "This is not a valid "
        << requiredType
        << " XML hierarchy.");
    return SQ_EXIT_ERROR;
    }

  /// build the pipeline
  vtkPVXMLElement *elem;

  // set up reader
  vector<string> arrays;
  vtkSQBOVReader *r=vtkSQBOVReader::New();
  if (r->Initialize(root,bovFileName,arrays))
    {
    r->Delete();
    sqErrorMacro(pCerr(),"Failed to initialize reader.");
    return SQ_EXIT_ERROR;
    }
  if (arrays.size()!=1)
    {
    sqErrorMacro(pCerr(),"Expected one vector array, got " << arrays.size());
    return SQ_EXIT_ERROR;
    }
  r->SetPointArrayStatus(arrays[0].c_str(),1);

  // terminator surfaces
  vtkSQHemisphereSource *hs=vtkSQHemisphereSource::New();
  if (hs->Initialize(root))
    {
    // termination surfaces are not neccessary when using
    // in displacement map mode. assume its ok for now
    // verify later.
    hs->Delete();
    hs=0;

    // r->Delete();
    //sqErrorMacro(pCerr(),"Failed to initialize terminator surfaces.");
    //return SQ_EXIT_ERROR;
    }

  // seed source
  iErr=0;
  vtkAlgorithm *ss=0;
  vtkSQPlaneSource *ps=vtkSQPlaneSource::New();
  vtkSQVolumeSource *vs=vtkSQVolumeSource::New();
  const char *outFileExt;
  if (!ps->Initialize(root))
    {
    // 2D source
    outFileExt=".pvtp";
    vs->Delete();
    ss=ps;
    }
  else
  if (!vs->Initialize(root))
    {
    // 3D source
    outFileExt=".pvtu";
    ps->Delete();
    ss=vs;
    }
  else
    {
    r->Delete();
    if (hs){ hs->Delete(); }
    vs->Delete();
    ps->Delete();
    sqErrorMacro(pCerr(),"Failed to initialize seeds.");
    return SQ_EXIT_ERROR;
    }

  // field topology mapper
  vtkSQFieldTracer *ftm=vtkSQFieldTracer::New();
  if (ftm->Initialize(root))
    {
    r->Delete();
    if (hs){ hs->Delete(); }
    ss->Delete();
    ftm->Delete();
    sqErrorMacro(pCerr(),"Failed to initialize field tracer.");
    return SQ_EXIT_ERROR;
    }
  ftm->SetSqueezeColorMap(0);

  ftm->AddVectorInputConnection(r->GetOutputPort(0));
  if (hs)
    {
    ftm->AddTerminatorInputConnection(hs->GetOutputPort(0));
    ftm->AddTerminatorInputConnection(hs->GetOutputPort(1));
    }

  ftm->AddSeedPointInputConnection(ss->GetOutputPort(0));
  ftm->SetInputArrayToProcess(
        0,
        0,
        0,
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        arrays[0].c_str());


  vtkXMLPDataSetWriter *w=vtkXMLPDataSetWriter::New();
  //w->SetDataModeToBinary();
  w->SetDataModeToAppended();
  w->SetEncodeAppendedData(0);
  w->SetCompressorTypeToNone();
  w->AddInputConnection(0,ftm->GetOutputPort(0));
  w->SetNumberOfPieces(worldSize);
  w->SetStartPiece(worldRank);
  w->SetEndPiece(worldRank);
  w->UpdateInformation();
  //const char *ext=w->GetDefaultFileExtension();
  ftm->Delete();

  // initialize for domain decomposition
  vtkStreamingDemandDrivenPipeline* exec
    = dynamic_cast<vtkStreamingDemandDrivenPipeline*>(ftm->GetExecutive());

  vtkInformation *info=exec->GetOutputInformation(0);

  exec->SetUpdateNumberOfPieces(info,worldSize);
  exec->SetUpdatePiece(info,worldRank);
  exec->SetUpdateExtent(info,worldRank,worldSize,0);

  // querry available times
  double *timeInfo=vtkStreamingDemandDrivenPipeline::TIME_STEPS()->Get(info);
  int nTimes=vtkStreamingDemandDrivenPipeline::TIME_STEPS()->Length(info);
  if (nTimes<1)
    {
    sqErrorMacro(pCerr(),"Error: No timesteps.");
    if (hs) { hs->Delete(); }
    r->Delete();
    ss->Delete();
    w->Delete();
    return SQ_EXIT_ERROR;
    }
  vector<double> times(timeInfo,timeInfo+nTimes);

  int startTimeIdx;
  int endTimeIdx;
  if (startTime<0.0)
    {
    // if no start time was provided use entire series
    startTime=times[0];
    startTimeIdx=0;

    endTime=times[nTimes-1];
    endTimeIdx=nTimes-1;
    }
  else
    {
    // get indices of requested start and end times
    startTimeIdx=IndexOf(startTime,&times[0],0,nTimes-1);
    if (startTimeIdx<0)
      {
      sqErrorMacro(pCerr(),"Invalid start time " << startTimeIdx << ".");
      if (hs) { hs->Delete(); }
      r->Delete();
      ss->Delete();
      w->Delete();
      return SQ_EXIT_ERROR;
      }

    endTimeIdx=IndexOf(endTime,&times[0],0,nTimes-1);
    if (endTimeIdx<0)
      {
      if (hs) { hs->Delete(); }
      r->Delete();
      ss->Delete();
      w->Delete();
      sqErrorMacro(pCerr(),"Invalid end time " << endTimeIdx << ".");
      return SQ_EXIT_ERROR;
      }
    }

  if (worldRank==0)
    {
    pCerr()
      << "Selected "
      << startTime << ":" << startTimeIdx
      << " to "
      << endTime << ":" << endTimeIdx
      << endl;
    }

  // make a directory for this dataset
  if (worldRank==0)
    {
    iErr=mkdir(outputPath,S_IRWXU|S_IXGRP);
    if (iErr<0 && (errno!=EEXIST))
      {
      char *sErr=strerror(errno);
      sqErrorMacro(pCerr(),
          << "Failed to mkdir " << outputPath << "." << endl
          << "Error: " << sErr << ".");
      if (hs) { hs->Delete(); }
      r->Delete();
      ss->Delete();
      w->Delete();
      return SQ_EXIT_ERROR;
      }
    }
  MPI_Barrier(MPI_COMM_WORLD);

  /// execute
  // run the pipeline for each time step, write the
  // result to disk.
  for (int idx=startTimeIdx,q=0; idx<=endTimeIdx; ++idx,++q)
    {
    double time=times[idx];

    exec->SetUpdateTimeStep(0,time);

    ostringstream fns;
    fns
      << outputPath
      << "/"
      << setfill('0') << setw(8) << time;

    // make a directory for this time step
    if (worldRank==0)
      {
      iErr=mkdir(fns.str().c_str(),S_IRWXU|S_IXGRP);
      if (iErr<0 && (errno!=EEXIST))
        {
        char *sErr=strerror(errno);
        sqErrorMacro(pCerr(),
            << "Failed to mkdir " << fns.str() << "."
            << "Error: " << sErr << ".");
        if (hs) { hs->Delete(); }
        r->Delete();
        ss->Delete();
        w->Delete();
        return SQ_EXIT_ERROR;
        }
      }
    MPI_Barrier(MPI_COMM_WORLD);

    fns << "/mt" << outFileExt;

    w->SetFileName(fns.str().c_str());
    w->Write();

    if (worldRank==0)
      {
      pCerr() << "Wrote: " << fns.str().c_str() << "." << endl;
      }
    }

  if (hs) { hs->Delete(); }
  r->Delete();
  ss->Delete();
  w->Delete();

  free(configName);

  vtkAlgorithm::SetDefaultExecutivePrototype(0);

  controller->Finalize();

  return SQ_EXIT_SUCCESS;
}
