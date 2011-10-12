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
#include "vtkDataObject.h"

#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkXMLPDataSetWriter.h"

#include "SQMacros.h"
#include "postream.h"
#include "vtkSQBOVReader.h"
#include "vtkSQImageGhosts.h"
#include "vtkSQKernelConvolution.h"
#include "XMLUtils.h"
#include "DebugUtil.h"

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
  vtkPVXMLElement *elem;

  // reader
  elem=GetRequiredElement(root,"vtkSQBOVReader");
  if (elem==0)
    {
    return SQ_EXIT_ERROR;
    }

  vtkSQBOVReader *r=vtkSQBOVReader::New();

  // set MPI optimizations 
  int cb_enable=0;
  int iErr=0;
  iErr=GetOptionalAttribute<int,1>(elem,"cb_enable",&cb_enable);
  if (iErr!=0)
    {
    sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
    return SQ_EXIT_ERROR;
    }

  if (cb_enable==0)
    {
    r->SetUseCollectiveIO(vtkSQBOVReader::HINT_DISABLED);
    }
  else
  if (cb_enable>0)
    {
    r->SetUseCollectiveIO(vtkSQBOVReader::HINT_ENABLED);
    }
  else
    {
    r->SetUseCollectiveIO(vtkSQBOVReader::HINT_AUTOMATIC);
    }


  r->SetUseDataSieving(vtkSQBOVReader::HINT_AUTOMATIC);
  r->SetFileName(bovFileName);
  if (!r->IsOpen())
    {
    return SQ_EXIT_ERROR;
    }

  // subset the data
  // when the user passes -1, we'll use the whole extent
  iErr=0;
  int ISubset[2]={-1,-1};
  iErr+=GetOptionalAttribute<int,2>(elem,"ISubset",ISubset);

  int JSubset[2]={-1,-1};
  iErr+=GetOptionalAttribute<int,2>(elem,"JSubset",JSubset);

  int KSubset[2]={-1,-1};
  iErr+=GetOptionalAttribute<int,2>(elem,"KSubset",KSubset);

  if (iErr!=0)
    {
    sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
    return SQ_EXIT_ERROR;
    }

  int IWholeset[2]={-1,-1};
  r->GetISubsetRange(IWholeset);
  if (ISubset[0]<0)
    {
    ISubset[0]=IWholeset[0];
    }
  if (ISubset[1]<0)
    {
    ISubset[1]=IWholeset[1];
    }
  r->SetISubset(ISubset[0],ISubset[1]);

  int JWholeset[2]={-1,-1};
  r->GetJSubsetRange(JWholeset);
  if (JSubset[0]<0)
    {
    JSubset[0]=JWholeset[0];
    }
  if (JSubset[1]<0)
    {
    JSubset[1]=JWholeset[1];
    }
  r->SetJSubset(JSubset[0],JSubset[1]);

  int KWholeset[2]={-1,-1};
  r->GetKSubsetRange(KWholeset);
  if (KSubset[0]<0)
    {
    KSubset[0]=KWholeset[0];
    }
  if (KSubset[1]<0)
    {
    KSubset[1]=KWholeset[1];
    }
  r->SetKSubset(KSubset[0],KSubset[1]);

  // select arrays to process
  // when none are provided we process all available
  vector<string> arrays;
  int nArrays=0;
  elem=GetOptionalElement(elem,"arrays");
  if (elem)
    {
    ExtractValues(elem->GetCharacterData(),arrays);
    nArrays=arrays.size();
    if (nArrays<1)
      {
      sqErrorMacro(pCerr(),"Error: parsing <arrays>.");
      return SQ_EXIT_ERROR;
      }
    }
  else
    {
    nArrays=r->GetNumberOfPointArrays();
    for (int i=0; i<nArrays; ++i)
      {
      arrays.push_back(r->GetPointArrayName(i));
      }
    }

  // set up ghost cell generator
  vtkSQImageGhosts *ig=vtkSQImageGhosts::New();
  ig->AddInputConnection(r->GetOutputPort(0));
  r->Delete();

  // set up kernel convolution filter
  iErr=0;
  elem=GetRequiredElement(root,"vtkSQKernelConvolution");
  if (elem==0)
    {
    return SQ_EXIT_ERROR;
    }

  int stencilWidth;
  iErr+=GetRequiredAttribute<int,1>(elem,"stencilWidth",&stencilWidth);

  if (iErr!=0)
    {
    sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
    return SQ_EXIT_ERROR;
    }

  vtkSQKernelConvolution *kconv=vtkSQKernelConvolution::New();
  kconv->SetKernelType(vtkSQKernelConvolution::KERNEL_TYPE_GAUSIAN);
  kconv->SetKernelWidth(stencilWidth);
  kconv->AddInputConnection(ig->GetOutputPort(0));
  ig->Delete();

  vtkXMLPDataSetWriter *w=vtkXMLPDataSetWriter::New();
  //w->SetDataModeToBinary();
  w->SetDataModeToAppended();
  w->SetEncodeAppendedData(0);
  w->SetCompressorTypeToNone();
  w->AddInputConnection(0,kconv->GetOutputPort(0));
  w->SetNumberOfPieces(worldSize);
  w->SetStartPiece(worldRank);
  w->SetEndPiece(worldRank);
  w->UpdateInformation();
  //const char *ext=w->GetDefaultFileExtension();
  kconv->Delete();

  // initialize for domain decomposition
  vtkStreamingDemandDrivenPipeline* exec
    = dynamic_cast<vtkStreamingDemandDrivenPipeline*>(r->GetExecutive());

  vtkInformation *info=exec->GetOutputInformation(0);

  exec->SetUpdateNumberOfPieces(info,worldSize);
  exec->SetUpdatePiece(info,worldRank);
  exec->SetUpdateExtent(info,worldRank,worldSize,0);

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
      << endl;
    }

  root->Delete();

  // make a directory for this dataset
  iErr=mkdir(outputPath,S_IRWXU|S_IXGRP);
  if (iErr<0 && (errno!=EEXIST))
    {
    char *sErr=strerror(errno);
    sqErrorMacro(pCerr(),
        << "Failed to mkdir " << outputPath << "." << endl
        << "Error: " << sErr << ".");
    return SQ_EXIT_ERROR;
    }

  /// execute
  // run the pipeline for each time step, write the
  // result to disk.
  for (int idx=startTimeIdx,q=0; idx<=endTimeIdx; ++idx,++q)
    {
    double time=times[idx];

    if (worldRank==0)
      {
      pCerr() << "processing time " << time << endl;
      }

    exec->SetUpdateTimeStep(0,time);

    // make a directory for this time step
    ostringstream fns;
    fns.str("");
    fns
      << outputPath
      << "/"
      << setfill('0') << setw(8) << (unsigned int)time;

    iErr=mkdir(fns.str().c_str(),S_IRWXU|S_IXGRP);
    if (iErr<0 && (errno!=EEXIST))
      {
      char *sErr=strerror(errno);
      sqErrorMacro(pCerr(),
          << "Failed to mkdir " << fns.str() << "."
          << "Error: " << sErr << ".");
      return SQ_EXIT_ERROR;
      }

    // loop over requested arrays
    for (int vecIdx=0; vecIdx<nArrays; ++vecIdx)
      {
      const string &vecName=arrays[vecIdx];

      fns.str("");
      fns
        << outputPath
        << "/"
        << setfill('0') << setw(8) << (unsigned int)time
        << "/"
        << vecName;

      // make a directory for this vector
      iErr=mkdir(fns.str().c_str(),S_IRWXU|S_IXGRP);
      if (iErr<0 && (errno!=EEXIST))
        {
        char *sErr=strerror(errno);
        sqErrorMacro(pCerr(),
            << "Failed to mkdir " << fns.str() << "."
            << "Error: " << sErr << ".");
        return SQ_EXIT_ERROR;
        }

      r->ClearPointArrayStatus();
      r->SetPointArrayStatus(vecName.c_str(),1);

      kconv->SetInputArrayToProcess(
            0,
            0,
            0,
            vtkDataObject::FIELD_ASSOCIATION_POINTS,
            vecName.c_str());

      fns
        << "/"
        << vecName
        << ".pvti";

      w->SetFileName(fns.str().c_str());
      w->Write();

      if (worldRank==0)
        {
        pCerr() << "Wrote: " << fns.str().c_str() << "." << endl;
        }
      }
    }
  w->Delete();

  free(configName);
  free(outputPath);
  free(bovFileName);

  vtkAlgorithm::SetDefaultExecutivePrototype(0);

  controller->Finalize();

  return SQ_EXIT_SUCCESS;
}
