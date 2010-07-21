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



//#include "vtkPointData.h"
//#include "vtkCellData.h"
//#include "vtkPolyData.h"


#include <sstream>
using std::istringstream;
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
template<typename T>
T GetRequiredAttribute(vtkPVXMLElement *elem, const char *attName)
{
  const char *attValue=elem->GetAttribute(attName);
  if (attValue==NULL)
    {
    sqErrorMacro(pCerr(),"No attribute named " << attName);
    exit(SQ_EXIT_ERROR);
    }
  T val;
  istringstream is(attValue);
  is >> val;
  return val;
}

//*****************************************************************************
void GetRequiredAttribute(
      vtkPVXMLElement *elem,
      const char *attName,
      const char **attValue)
{
  *attValue=elem->GetAttribute(attName);
  if (*attValue==NULL)
    {
    sqErrorMacro(pCerr(),"No attribute named " << attName);
    exit(SQ_EXIT_ERROR);
    }
}

//*****************************************************************************
template<typename T, int N>
void GetRequiredAttribute(
      vtkPVXMLElement *elem,
      const char *attName,
      T *attValue)
{
  const char *attValueStr=elem->GetAttribute(attName);
  if (attValueStr==NULL)
    {
    sqErrorMacro(pCerr(),"No attribute named " << attName << ".");
    exit(SQ_EXIT_ERROR);
    }

  T *pAttValue=attValue;
  istringstream is(attValueStr);
  for (int i=0; i<N; ++i)
    {
    if (!is.good())
      {
      sqErrorMacro(pCerr(),"Wrong number of values in " << attName <<".");
      exit(SQ_EXIT_ERROR);
      }
    is >> *pAttValue;
    ++pAttValue;
    }
}

// //*****************************************************************************
// template <>
// const char *GetRequiredAttribute<char *,1>(
//       vtkPVXMLElement *elem,
//       const char *attName,
//       char **attValue)
// {
//   *attValue=elem->GetAttribute(attName);
//   if (*attValue==NULL)
//     {
//     sqErrorMacro(pCerr(),"No attribute named " << attName);
//     exit(SQ_EXIT_ERROR);
//     }
// }


//*****************************************************************************
vtkPVXMLElement *GetRequiredElement(
      vtkPVXMLElement *root,
      const char *name)
{
  vtkPVXMLElement *elem=root->FindNestedElementByName(name);
  if (elem==0)
    {
    sqErrorMacro(pCerr(),"No element named " << name << ".");
    exit(SQ_EXIT_ERROR);
    }
  return elem;
}


//*****************************************************************************
vtkPVXMLElement *GetOptionalElement(
      vtkPVXMLElement *root,
      const char *name)
{
  vtkPVXMLElement *elem=root->FindNestedElementByName(name);
  return elem;
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
  ///MPI_Init(&argc,&argv);
  vtkMPIController* controller=vtkMPIController::New();
  controller->Initialize(&argc,&argv,0);
  int worldRank=controller->GetLocalProcessId();
  int worldSize=controller->GetNumberOfProcesses();

  vtkMultiProcessController::SetGlobalController(controller);

  vtkCompositeDataPipeline* cexec=vtkCompositeDataPipeline::New();
  vtkAlgorithm::SetDefaultExecutivePrototype(cexec);
  cexec->Delete();


  //controller->Delete();

  if (argc<2)
    {
    if (worldRank==0)
      {
      sqErrorMacro(cerr,"$2 must contain the path to a configuration file.");
      }
    return SQ_EXIT_ERROR;
    }

  // distribute the configuration file name.
  int configNameLen=0;
  char *configName=0;
  if (worldRank==0)
    {
    configNameLen=strlen(argv[1]);
    controller->Broadcast(&configNameLen,1,0);
    char *configName=(char *)malloc(configNameLen);
    strncpy(configName,argv[1],configNameLen);
    controller->Broadcast(configName,configNameLen,0);
    }
  else
    {
    controller->Broadcast(&configNameLen,1,0);
    char *configName=(char *)malloc(configNameLen);
    controller->Broadcast(configName,configNameLen,0);
    }

  // read the configuration file.
  vtkPVXMLParser *parser=vtkPVXMLParser::New();
  parser->SetFileName(argv[1]);
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
        << "This is not a valid " << requiredType
        << " XML hierarchy.");
    return SQ_EXIT_ERROR;
    }

  /// build the pipeline
  vtkPVXMLElement *elem;

  // reader
  elem=GetRequiredElement(root,"vtkSQBOVReader");

  const char *bovFileName;
  GetRequiredAttribute(elem,"bov_file_name",&bovFileName);

  double startTime;
  GetRequiredAttribute<double,1>(elem,"start_time",&startTime);

  double endTime;
  GetRequiredAttribute<double,1>(elem,"end_time",&endTime);

  const char *vectors;
  GetRequiredAttribute(elem,"vectors",&vectors);

  int decompDims[3];
  GetRequiredAttribute<int,3>(elem,"decomp_dims",decompDims);

  int blockCacheSize;
  GetRequiredAttribute<int,1>(elem,"block_cache_size",&blockCacheSize);

  vtkSQBOVReader *r=vtkSQBOVReader::New();
  r->SetMetaRead(1);
  r->SetUseCollectiveIO(vtkSQBOVReader::HINT_DISABLED);
  r->SetUseDataSieving(vtkSQBOVReader::HINT_AUTOMATIC);
  r->SetFileName(bovFileName);
  r->SetPointArrayStatus(vectors,1);
  r->SetDecompDims(decompDims);
  r->SetBlockCacheSize(blockCacheSize);

  // earth terminator surfaces
  elem=GetRequiredElement(root,"vtkSQHemisphereSource");

  double hemiCenter[3];
  GetRequiredAttribute<double,3>(elem,"center",hemiCenter);

  double hemiRadius;
  GetRequiredAttribute<double,1>(elem,"radius",&hemiRadius);

  int hemiResolution;
  GetRequiredAttribute<int,1>(elem,"resolution",&hemiResolution);

  vtkSQHemisphereSource *hs=vtkSQHemisphereSource::New();
  hs->SetCenter(hemiCenter);
  hs->SetRadius(hemiRadius);
  hs->SetResolution(hemiResolution);

  // seed source
  vtkAlgorithm *ss;
  const char *outFileExt;
  if ((elem=GetOptionalElement(root,"vtkSQPlaneSource"))!=NULL)
    {
    // 2D source
    outFileExt=".pvtp";

    double origin[3];
    GetRequiredAttribute<double,3>(elem,"origin",origin);

    double point1[3];
    GetRequiredAttribute<double,3>(elem,"point1",point1);

    double point2[3];
    GetRequiredAttribute<double,3>(elem,"point2",point2);

    int resolution[2];
    GetRequiredAttribute<int,2>(elem,"resolution",resolution);

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
    GetRequiredAttribute<double,3>(elem,"origin",origin);

    double point1[3];
    GetRequiredAttribute<double,3>(elem,"point1",point1);

    double point2[3];
    GetRequiredAttribute<double,3>(elem,"point2",point2);

    double point3[3];
    GetRequiredAttribute<double,3>(elem,"point3",point3);

    int resolution[3];
    GetRequiredAttribute<int,3>(elem,"resolution",resolution);

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
  elem=GetRequiredElement(root,"vtkSQFieldTracer");

  int integratorType;
  GetRequiredAttribute<int,1>(elem,"integrator_type",&integratorType);

  double maxStepSize;
  GetRequiredAttribute<double,1>(elem,"max_step_size",&maxStepSize);

  double maxArcLength;
  GetRequiredAttribute<double,1>(elem,"max_arc_length",&maxArcLength);

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
//   ftm->AddInputConnection(0,r->GetOutputPort(0));
//   ftm->AddInputConnection(1,hs->GetOutputPort(0));
//   ftm->AddInputConnection(2,ss->GetOutputPort(0));
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
  elem=GetRequiredElement(root,"vtkXMLPDataSetWriter");

  const char *outFileBase;
  GetRequiredAttribute(elem,"out_file_base",&outFileBase);

  vtkXMLPDataSetWriter *w=vtkXMLPDataSetWriter::New();
  w->SetDataModeToBinary();
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

  // pvd file stream
  ofstream pvds;
  if (worldRank==0)
    {
    string pvdFileName;
    pvdFileName+=outFileBase;
    pvdFileName+=".pvd";

    pvds.open(pvdFileName.c_str());
    if (!pvds.good())
      {
      sqErrorMacro(pCerr(),"Failed to open " << pvdFileName << ".");
      return SQ_EXIT_ERROR;
      }

    pvds
      << "<?xml version=\"1.0\"?>" << endl
      << "<VTKFile type=\"Collection\">" << endl
      << "<Collection>" << endl;
    }

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

    if (worldRank==0)
      {
      pvds
        << "<DataSet "
        << "timestep=\"" << time << "\" "
        << "group=\"\" part=\"0\" "
        << "file=\"" << fn.c_str() << "\""
        << "/>"
        << endl;
      }
    }

  if (worldRank==0)
    {
    pvds
      << "</Collection>" << endl
      << "</VTKFile>" << endl
      << endl;
    pvds.close();
    }

  w->Delete();
  parser->Delete();

  ///MPI_Finalize();
  controller->Finalize();
  controller->Delete();
  vtkAlgorithm::SetDefaultExecutivePrototype(0);

  return SQ_EXIT_SUCCESS;
}
