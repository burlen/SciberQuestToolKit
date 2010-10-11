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

  if (argc<4)
    {
    if (worldRank==0)
      {
      pCerr()
        << "Error: Command tail." << endl
        << " 1) /path/to/runConfig.xml" << endl
        << " 2) startTime" << endl
        << " 3) endTime" << endl
        << endl;
      }
    return SQ_EXIT_ERROR;
    }

  // distribute the configuration file name and time range
  int configNameLen=0;
  char *configName=0;
  double startTime=-1.0;
  double endTime=-1.0;

  if (worldRank==0)
    {
    configNameLen=strlen(argv[1]);
    char *configName=(char *)malloc(configNameLen);
    strncpy(configName,argv[1],configNameLen);
    controller->Broadcast(&configNameLen,1,0);
    controller->Broadcast(configName,configNameLen,0);

    startTime=atof(argv[2]);
    endTime=atof(argv[3]);
    controller->Broadcast(&startTime,1,0);
    controller->Broadcast(&endTime,1,0);
    }
  else
    {
    controller->Broadcast(&configNameLen,1,0);
    char *configName=(char *)malloc(configNameLen);
    controller->Broadcast(configName,configNameLen,0);
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

  // reader
  elem=GetRequiredElement(root,"vtkSQBOVReader");
  if (elem==0)
    {
    return SQ_EXIT_ERROR;
    }

  iErr=0;
  const char *bovFileName;
  iErr+=GetRequiredAttribute(elem,"bov_file_name",&bovFileName);

  // double startTime;
  // GetRequiredAttribute<double,1>(elem,"start_time",&startTime);
  //
  // double endTime;
  // GetRequiredAttribute<double,1>(elem,"end_time",&endTime);

  const char *vectors;
  iErr+=GetRequiredAttribute(elem,"vectors",&vectors);

  int decompDims[3];
  iErr+=GetRequiredAttribute<int,3>(elem,"decomp_dims",decompDims);

  int blockCacheSize;
  iErr+=GetRequiredAttribute<int,1>(elem,"block_cache_size",&blockCacheSize);

  if (iErr!=0)
    {
    sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
    return SQ_EXIT_ERROR;
    }

  vtkSQBOVReader *r=vtkSQBOVReader::New();
  r->SetMetaRead(1);
  r->SetUseCollectiveIO(vtkSQBOVReader::HINT_DISABLED);
  r->SetUseDataSieving(vtkSQBOVReader::HINT_AUTOMATIC);
  r->SetFileName(bovFileName);
  r->SetPointArrayStatus(vectors,1);
  r->SetDecompDims(decompDims);
  r->SetBlockCacheSize(blockCacheSize);

  // earth terminator surfaces
  iErr=0;
  elem=GetRequiredElement(root,"vtkSQHemisphereSource");
  if (elem==0)
    {
    return SQ_EXIT_ERROR;
    }

  double hemiCenter[3];
  iErr+=GetRequiredAttribute<double,3>(elem,"center",hemiCenter);

  double hemiRadius;
  iErr+=GetRequiredAttribute<double,1>(elem,"radius",&hemiRadius);

  int hemiResolution;
  iErr+=GetRequiredAttribute<int,1>(elem,"resolution",&hemiResolution);

  if (iErr!=0)
    {
    sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
    return SQ_EXIT_ERROR;
    }

  vtkSQHemisphereSource *hs=vtkSQHemisphereSource::New();
  hs->SetCenter(hemiCenter);
  hs->SetRadius(hemiRadius);
  hs->SetResolution(hemiResolution);

  // seed source
  iErr=0;
  vtkAlgorithm *ss;
  const char *outFileExt;
  if ((elem=GetOptionalElement(root,"vtkSQPlaneSource"))!=NULL)
    {
    // 2D source
    outFileExt=".pvtp";

    double origin[3];
    iErr+=GetRequiredAttribute<double,3>(elem,"origin",origin);

    double point1[3];
    iErr+=GetRequiredAttribute<double,3>(elem,"point1",point1);

    double point2[3];
    iErr+=GetRequiredAttribute<double,3>(elem,"point2",point2);

    int resolution[2];
    iErr+=GetRequiredAttribute<int,2>(elem,"resolution",resolution);

    if (iErr!=0)
      {
      sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
      return SQ_EXIT_ERROR;
      }

    vtkSQPlaneSource *ps=vtkSQPlaneSource::New();
    ps->SetOrigin(origin);
    ps->SetPoint1(point1);
    ps->SetPoint2(point2);
    ps->SetResolution(resolution);
    ss=ps;
    }
  else
  if ((elem=GetOptionalElement(root,"vtkSQVolumeSource"))!=NULL)
    {
    // 3D source
    outFileExt=".pvtu";

    double origin[3];
    iErr+=GetRequiredAttribute<double,3>(elem,"origin",origin);

    double point1[3];
    iErr+=GetRequiredAttribute<double,3>(elem,"point1",point1);

    double point2[3];
    iErr+=GetRequiredAttribute<double,3>(elem,"point2",point2);

    double point3[3];
    iErr+=GetRequiredAttribute<double,3>(elem,"point3",point3);

    int resolution[3];
    iErr+=GetRequiredAttribute<int,3>(elem,"resolution",resolution);

    if (iErr!=0)
      {
      sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
      return SQ_EXIT_ERROR;
      }

    vtkSQVolumeSource *vs=vtkSQVolumeSource::New();
    vs->SetOrigin(origin);
    vs->SetPoint1(point1);
    vs->SetPoint2(point2);
    vs->SetPoint3(point3);
    vs->SetResolution(resolution);
    ss=vs;
    }
  else
    {
    sqErrorMacro(pCerr(),"No seed source found.");
    return SQ_EXIT_ERROR;
    }

  // field topology mapper
  iErr=0;
  elem=GetRequiredElement(root,"vtkSQFieldTracer");
  if (elem==0)
    {
    return SQ_EXIT_ERROR;
    }

  int integratorType;
  iErr+=GetRequiredAttribute<int,1>(elem,"integrator_type",&integratorType);

  double maxStepSize;
  iErr+=GetRequiredAttribute<double,1>(elem,"max_step_size",&maxStepSize);

  double maxArcLength;
  iErr+=GetRequiredAttribute<double,1>(elem,"max_arc_length",&maxArcLength);

  double minStepSize;
  int maxNumberSteps;
  double maxError;
  if (integratorType==vtkSQFieldTracer::INTEGRATOR_RK45)
    {
    GetRequiredAttribute<double,1>(elem,"min_step_size",&minStepSize);
    GetRequiredAttribute<int,1>(elem,"max_number_steps",&maxNumberSteps);
    GetRequiredAttribute<double,1>(elem,"max_error",&maxError);
    }

  double nullThreshold;
  GetRequiredAttribute<double,1>(elem,"null_threshold",&nullThreshold);

  int dynamicScheduler;
  GetRequiredAttribute<int,1>(elem,"dynamic_scheduler",&dynamicScheduler);

  int masterBlockSize;
  int workerBlockSize;
  if (dynamicScheduler)
    {
    GetRequiredAttribute<int,1>(elem,"master_block_size",&masterBlockSize);
    GetRequiredAttribute<int,1>(elem,"worker_block_size",&workerBlockSize);
    }

  if (iErr!=0)
    {
    sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
    return SQ_EXIT_ERROR;
    }

  vtkSQFieldTracer *ftm=vtkSQFieldTracer::New();
  ftm->SetMode(vtkSQFieldTracer::MODE_TOPOLOGY);
  ftm->SetIntegratorType(integratorType);
  ftm->SetMaxLineLength(maxArcLength);
  ftm->SetMaxStep(maxStepSize);
  if (integratorType==vtkSQFieldTracer::INTEGRATOR_RK45)
    {
    ftm->SetMinStep(minStepSize);
    ftm->SetMaxNumberOfSteps(maxNumberSteps);
    ftm->SetMaxError(maxError);
    }
  ftm->SetNullThreshold(nullThreshold);
  ftm->SetUseDynamicScheduler(dynamicScheduler);
  if (dynamicScheduler)
    {
    ftm->SetMasterBlockSize(masterBlockSize);
    ftm->SetWorkerBlockSize(workerBlockSize);
    }
  ftm->SetSqueezeColorMap(0);
  ftm->AddVectorInputConnection(r->GetOutputPort(0));
  ftm->AddTerminatorInputConnection(hs->GetOutputPort(0));
  ftm->AddTerminatorInputConnection(hs->GetOutputPort(1));
  ftm->AddSeedPointInputConnection(ss->GetOutputPort(0));
  ftm->SetInputArrayToProcess(
        0,
        0,
        0,
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vectors);

  r->Delete();
  hs->Delete();
  ss->Delete();

  // writer.
  iErr=0;
  elem=GetRequiredElement(root,"vtkXMLPDataSetWriter");
  if (elem==0)
    {
    return SQ_EXIT_ERROR;
    }

  const char *outFileBase;
  iErr+=GetRequiredAttribute(elem,"out_file_base",&outFileBase);

  if (iErr!=0)
    {
    sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
    return SQ_EXIT_ERROR;
    }

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

  // querry available times
  exec->UpdateInformation();
  double *times=vtkStreamingDemandDrivenPipeline::TIME_STEPS()->Get(info);
  int nTimes=vtkStreamingDemandDrivenPipeline::TIME_STEPS()->Length(info);

  int startTimeIdx=IndexOf(startTime,times,0,nTimes-1);
  if (startTimeIdx<0)
    {
    sqErrorMacro(pCerr(),"Invalid start time " << startTimeIdx << ".");
    return SQ_EXIT_ERROR;
    }

  int endTimeIdx=IndexOf(endTime,times,0,nTimes-1);
  if (endTimeIdx<0)
    {
    sqErrorMacro(pCerr(),"Invalid end time " << endTimeIdx << ".");
    return SQ_EXIT_ERROR;
    }

  /// execute
  // run the pipeline for each time step, write the
  // result to disk.
  for (int idx=startTimeIdx,q=0; idx<=endTimeIdx; ++idx,++q)
    {
    double time=times[idx];

    exec->SetUpdateTimeStep(0,time);

    ostringstream fns;
    fns
      << outFileBase << "_"
      << setfill('0') << setw(8) << time
      << outFileExt;

    string fn=fns.str();

    w->SetFileName(fn.c_str());
    w->Write();
    }
  w->Delete();

  controller->Finalize();
  vtkAlgorithm::SetDefaultExecutivePrototype(0);

  return SQ_EXIT_SUCCESS;
}
