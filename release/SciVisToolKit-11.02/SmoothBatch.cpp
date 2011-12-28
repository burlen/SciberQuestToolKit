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
#include "vtkInformationVector.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkSmartPointer.h"
#include "vtkDataObject.h"

#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkXMLPDataSetWriter.h"

#include "SQMacros.h"
#include "postream.h"
#include "vtkSQBOVReader.h"
#include "vtkSQBOVWriter.h"
#include "vtkSQImageGhosts.h"
#include "vtkSQKernelConvolution.h"
#include "vtkSQLog.h"
#include "FsUtils.h"
#include "XMLUtils.h"
#include "DebugUtil.h"
#include "Tuple.hxx"

#include <sstream>
using std::ostringstream;
#include <iostream>
using std::cin;
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
  return SQ_EXIT_ERROR;
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

  //GdbAttachRanks("xterm","0");

  if (argc<4)
    {
    if (worldRank==0)
      {
      pCerr()
        << "Error: Command tail." << endl
        << " 1) /path/to/runConfig.xml" << endl
        << " 2) /path/to/file.bov" << endl
        << " 3) /path/to/output/" << endl
        << " 4) startTime" << endl
        << " 5) endTime" << endl
        << endl;
      }
    vtkAlgorithm::SetDefaultExecutivePrototype(0);
    return SQ_EXIT_ERROR;
    }

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent(0,"SmoothBatch::TotalRunTime");
  log->StartEvent(0,"SmoothBatch::Initialize");

  // distribute the configuration file name and time range
  int configNameLen=0;
  char *configName=0;

  int configFileLen=0;
  char *configFile=0;

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

    // read the metadata file into a buffer.
    ifstream fs(configName);
    if (!fs.good())
    {
        sqErrorMacro(pCerr(),
          << "Error, failed to open " << configName << ".");
        return SQ_EXIT_ERROR;
    }
    // get length of the xml in file.
    fs.seekg(0,ios::end);
    configFileLen=fs.tellg();
    fs.seekg(0,ios::beg);

    // allocate memory:
    configFile=(char *)malloc(configFileLen+1);
    // read data as a block:
    fs.read(configFile,configFileLen);
    if (fs.fail()||fs.bad())
    {
        fs.close();
        free(configFile);
        sqErrorMacro(pCerr(),
          << "Error, failed to read " << configName << ".");
        return SQ_EXIT_ERROR;
    }
    fs.close();
    configFile[configFileLen]='\0';
    configFileLen+=1;
    controller->Broadcast(&configFileLen,1,0);
    controller->Broadcast(configFile,configFileLen,0);

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
    const char *startTimeStr="";
    const char *endTimeStr="";
    if (argc>4)
      {
      startTime=atof(argv[4]);
      endTime=atof(argv[5]);

      startTimeStr=argv[4];
      endTimeStr=argv[5];
      }
    controller->Broadcast(&startTime,1,0);
    controller->Broadcast(&endTime,1,0);
    }
  else
    {
    controller->Broadcast(&configNameLen,1,0);
    configName=(char *)malloc(configNameLen);
    controller->Broadcast(configName,configNameLen,0);

    controller->Broadcast(&configFileLen,1,0);
    configFile=(char *)malloc(configFileLen);
    controller->Broadcast(configFile,configFileLen,0);

    controller->Broadcast(&bovFileNameLen,1,0);
    bovFileName=(char *)malloc(bovFileNameLen);
    controller->Broadcast(bovFileName,bovFileNameLen,0);

    controller->Broadcast(&outputPathLen,1,0);
    outputPath=(char *)malloc(outputPathLen);
    controller->Broadcast(outputPath,outputPathLen,0);

    controller->Broadcast(&startTime,1,0);
    controller->Broadcast(&endTime,1,0);
    }

  *log
    << "# configName=" << configName << "\n"
    << "# bovFileName=" << bovFileName << "\n"
    << "# outputPath=" << outputPath << "\n"
    << "# timeRange=" << startTime << " " << endTime << "\n"
    << "\n";

  // read the configuration file.
  const string xml(configFile);
  free(configFile);

  istringstream xmls(xml);

  // parse the xml
  vtkPVXMLParser *parser=vtkPVXMLParser::New();
  parser->SetStream(&xmls);
  parser->Parse();

  // check for the semblance of a valid configuration hierarchy
  vtkPVXMLElement *root=parser->GetRootElement();
  if (root==0)
    {
    sqErrorMacro(pCerr(),"Invalid XML in file " << configName << ".");
    return SQ_EXIT_ERROR;
    }
  root->Register(0);
  parser->Delete();

  string requiredType("SmoothBatch");
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
  int iErr=0;

  // set up reader
  vector<string> arrays;
  vtkSQBOVReader *r=vtkSQBOVReader::New();
  iErr=r->Initialize(root,bovFileName,arrays);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"Failed to initialize reader.");
    return SQ_EXIT_ERROR;
    }

  // set up ghost cell generator
  vtkSQImageGhosts *ig=vtkSQImageGhosts::New();
  iErr=ig->Initialize(root);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"Failed to initialize ghost generator.");
    return SQ_EXIT_ERROR;
    }
  ig->AddInputConnection(r->GetOutputPort(0));
  r->Delete();

  // set up kernel convolution filter
  vtkSQKernelConvolution *kconv=vtkSQKernelConvolution::New();
  iErr=kconv->Initialize(root);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"Failed to initialize kernel convolution filter.");
    return SQ_EXIT_ERROR;
    }
  kconv->AddInputConnection(ig->GetOutputPort(0));
  ig->Delete();

  // set up the writer
  ostringstream bovId;
  bovId << "_sm_" << kconv->GetKernelWidth() << ".bov";
  string outputBovFileName
    = outputPath +
        StripExtensionFromFileName(StripPathFromFileName(bovFileName)) +
          bovId.str();

  vtkSQBOVWriter *w=vtkSQBOVWriter::New();
  iErr=w->Initialize(root);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"Failed to initialize writer.");
    return SQ_EXIT_ERROR;
    }
  w->SetFileName(outputBovFileName.c_str());
  w->AddInputConnection(0,kconv->GetOutputPort(0));
  kconv->Delete();

  // make a directory for this dataset
  if (worldRank==0)
    {
    iErr=mkdir(outputPath,S_IRWXU|S_IXGRP);
    if (iErr<0 && (errno!=EEXIST))
      {
      char *sErr=strerror(errno);
      sqErrorMacro(pCerr(),
          << "Failed to mkdir " << outputPath << "." << "\n"
          << "Error: " << sErr << ".");
      return SQ_EXIT_ERROR;
      }
    }

  // initialize for domain decomposition
  vtkStreamingDemandDrivenPipeline* exec
    = dynamic_cast<vtkStreamingDemandDrivenPipeline*>(kconv->GetExecutive());

  vtkInformation *info=exec->GetOutputInformation(0);

  exec->SetUpdateNumberOfPieces(info,worldSize);
  exec->SetUpdatePiece(info,worldRank);
  exec->SetUpdateExtent(info,worldRank,worldSize,0);

  //w->UpdateInformation();

  // querry available times
  exec->UpdateInformation();
  double *timeInfo=vtkStreamingDemandDrivenPipeline::TIME_STEPS()->Get(info);
  int nTimes=vtkStreamingDemandDrivenPipeline::TIME_STEPS()->Length(info);
  if (nTimes<1)
    {
    sqErrorMacro(pCerr(),"Error: No timesteps.");
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
      return SQ_EXIT_ERROR;
      }

    endTimeIdx=IndexOf(endTime,&times[0],0,nTimes-1);
    if (endTimeIdx<0)
      {
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
      << "\n";
    }

  root->Delete();

  // this barrier makes sure that directory for results exists before any
  // one actually tries to use it.
  MPI_Barrier(MPI_COMM_WORLD);
  log->EndEvent(0,"SmoothBatch::Initialize");

  /// execute
  // run the pipeline for each time step, write the
  // result to disk.
  for (int idx=startTimeIdx,q=0; idx<=endTimeIdx; ++idx,++q)
    {
    double time=times[idx];

    ostringstream oss;
    oss << idx << ":" << time;
    //string stepEventLabel("SmoothBatch::ProcessTimeStep_");
    //stepEventLabel+=oss.str();
    //log->StartEvent(stepEventLabel.c_str());

    if (worldRank==0)
      {
      pCerr() << "processing time " << time << "\n";
      }

    exec->SetUpdateTimeStep(0,time);

    // loop over requested arrays
    int nArrays=arrays.size();
    for (int vecIdx=0; vecIdx<nArrays; ++vecIdx)
      {
      const string &vecName=arrays[vecIdx];

      //string arrayEventLabel("SmoothBatch::ProcessArray_");
      //arrayEventLabel += vecName;
      //log->StartEvent(arrayEventLabel.c_str());

      r->ClearPointArrayStatus();
      r->SetPointArrayStatus(vecName.c_str(),1);

      kconv->SetInputArrayToProcess(
            0,
            0,
            0,
            vtkDataObject::FIELD_ASSOCIATION_POINTS,
            vecName.c_str());

      w->Update();

      if (worldRank==0)
        {
        pCerr() << "Wrote " << vecName << endl;
        }

      //log->EndEvent(arrayEventLabel.c_str());
      }

      //log->EndEvent(stepEventLabel.c_str());
    }

  w->WriteMetaData();
  w->Delete();

  free(configName);
  free(outputPath);
  free(bovFileName);


  MPI_Barrier(MPI_COMM_WORLD);
  log->EndEvent(0,"SmoothBatch::TotalRunTime");

  log->SetFileName("SmoothBatch.log");
  log->Update();
  log->Write();

  vtkAlgorithm::SetDefaultExecutivePrototype(0);

  controller->Finalize();

  return SQ_EXIT_SUCCESS;
}

