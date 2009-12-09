/*   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
/*=========================================================================

Program:   Visualization Toolkit
Module:    $RCSfile: vtkOOCDFieldTracer.cxx,v $

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOOCDFieldTracer.h"
// #define vtkOOCDFieldTracerDEBUG


#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkCellArray.h"
#include "vtkIdList.h"

#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include "vtkInitialValueProblemSolver.h"
#include "vtkRungeKutta45.h"
#include "vtkMultiProcessController.h"

#include "vtkOOCReader.h"
#include "vtkMetaDataKeys.h"
#include "vtkMath.h"

#ifdef PV_3_4_BUILD
  #include "vtkInterpolatedVelocityField-3.7.h"
#else
  #include "vtkInterpolatedVelocityField.h"
#endif

#include "FieldLine.h"
#include "TerminationCondition.h"
#include "WorkQueue.h"
#include "FieldTopologyMap.h"
#include "PolyDataFieldTopologyMap.h"
#include "UnstructuredFieldTopologyMap.h"
#include "TracerFieldTopologyMap.h"
#include "minmax.h"

#include "mpi.h"

#ifdef vtkOOCDFieldTracerTIME
  #include <sys/time.h>
#endif




vtkCxxRevisionMacro(vtkOOCDFieldTracer, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkOOCDFieldTracer);

const double vtkOOCDFieldTracer::EPSILON = 1.0E-12;

//-----------------------------------------------------------------------------
vtkOOCDFieldTracer::vtkOOCDFieldTracer()
      :
  UseDynamicScheduler(1),
  WorkerBlockSize(16),
  MasterBlockSize(256),
  StepUnit(CELL_FRACTION),
  InitialStep(0.1),
  MinStep(0.1),
  MaxStep(0.5),
  MaxError(1E-5),
  MaxNumberOfSteps(1000),
  MaxLineLength(1E6),
  NullThreshold(1E-6),
  OOCNeighborhoodSize(15),
  TopologyMode(0),
  SqueezeColorMap(0)
{
  this->Integrator=vtkRungeKutta45::New();
  this->TermCon=new TerminationCondition;
  this->Controller=vtkMultiProcessController::GetGlobalController();
  this->SetNumberOfInputPorts(3);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkOOCDFieldTracer::~vtkOOCDFieldTracer()
{
  this->Integrator->Delete();
  delete this->TermCon;
}

//-----------------------------------------------------------------------------
int vtkOOCDFieldTracer::FillInputPortInformation(int port, vtkInformation *info)
{
  #ifdef vtkOOCDFieldTracerDEBUG
  cerr << "====================================================================FillInputPortInformation" << endl;
  #endif
  switch (port)
    {
    // Vector feild data
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
      break;
    // Seed points
    case 1:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
      break;
    // termination Surface
    case 2:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
      info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(),1);
      info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(),1);
      break;
  default:
      vtkWarningMacro("Invalid input port " << port << " requested.");
      break;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkOOCDFieldTracer::FillOutputPortInformation(int port, vtkInformation *info)
{
  #ifdef vtkOOCDFieldTracerDEBUG
  cerr << "====================================================================FillOutputPortInformation" << endl;
  #endif

  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
  return 1;

  // NOTE my recolection is that this doesn't work quite right.
  switch (port)
    {
    case 0:
      if (this->TopologyMode)
        {
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
        }
      else
        {
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
        }
      break;
    default:
      vtkWarningMacro("Invalid output port requested.");
      break;
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkOOCDFieldTracer::AddVectorInputConnection(
                vtkAlgorithmOutput* algOutput)
{
  #ifdef vtkOOCDFieldTracerDEBUG
  cerr << "====================================================================AddDatasetInputConnectiont" << endl;
  #endif

  this->AddInputConnection(0, algOutput);
}

//----------------------------------------------------------------------------
void vtkOOCDFieldTracer::ClearVectorInputConnections()
{
  #ifdef vtkOOCDFieldTracerDEBUG
  cerr << "====================================================================ClearDatasetInputConnections" << endl;
  #endif

  this->SetInputConnection(0, 0);
}

//----------------------------------------------------------------------------
void vtkOOCDFieldTracer::AddSeedPointInputConnection(
                vtkAlgorithmOutput* algOutput)
{
  #ifdef vtkOOCDFieldTracerDEBUG
  cerr << "====================================================================AddSeedPointInputConnection" << endl;
  #endif
  this->AddInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void vtkOOCDFieldTracer::ClearSeedPointInputConnections()
{
  #ifdef vtkOOCDFieldTracerDEBUG
  cerr << "====================================================================ClearSeedPointInputConnections" << endl;
  #endif
  this->SetInputConnection(1, 0);
}

//----------------------------------------------------------------------------
void vtkOOCDFieldTracer::AddTerminatorInputConnection(
                vtkAlgorithmOutput* algOutput)
{
  #ifdef vtkOOCDFieldTracerDEBUG
  cerr << "====================================================================AddBoundaryInputConnection" << endl;
  #endif
  this->AddInputConnection(2, algOutput);
}

//----------------------------------------------------------------------------
void vtkOOCDFieldTracer::ClearTerminatorInputConnections()
{
  #ifdef vtkOOCDFieldTracerDEBUG
  cerr << "====================================================================ClearBoundaryInputConnections" << endl;
  #endif
  this->SetInputConnection(2, 0);
}

//-----------------------------------------------------------------------------
void vtkOOCDFieldTracer::SetStepUnit(int unit)
{ 
  if (unit!=ARC_LENGTH
   && unit!=CELL_FRACTION )
    {
    unit=CELL_FRACTION;
    }

  if (unit==this->StepUnit )
    {
    return;
    }
  this->StepUnit = unit;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkOOCDFieldTracer::RequestDataObject(
                vtkInformation *info,
                vtkInformationVector** inInfos,
                vtkInformationVector* outInfos)
{
  #ifdef vtkOOCDFieldTracerDEBUG
    cerr << "====================================================================RequestDataObject" << endl;
  #endif
  // get the filters output
  vtkInformation* outInfo = outInfos->GetInformationObject(0);
  vtkDataObject *outData=outInfo->Get(vtkDataObject::DATA_OBJECT());

  // If the filter is being run in TopologyMode the output is either
  // polydata or unstructured grid depending on the second input.
  // Otherwise the output is polydata.
  if (this->TopologyMode)
    {
    // duplicate the input type for the map output.
    vtkInformation* inInfo=inInfos[1]->GetInformationObject(0);
    vtkDataObject *inData=inInfo->Get(vtkDataObject::DATA_OBJECT());
    if (!outData || !outData->IsA(inData->GetClassName()))
      {
      outData=inData->NewInstance();
      outData->SetPipelineInformation(outInfo);
      outData->Delete();
      vtkInformation *portInfo=this->GetOutputPortInformation(0);
      portInfo->Set(vtkDataObject::DATA_EXTENT_TYPE(),VTK_PIECES_EXTENT);
      }
    }
  else
    {
    if (!outData)
      {
      // consrtruct a polydata for the field line output.
      outData=vtkPolyData::New();
      outData->SetPipelineInformation(outInfo);
      outData->Delete();
      vtkInformation *portInfo=this->GetOutputPortInformation(0);
      portInfo->Set(vtkDataObject::DATA_EXTENT_TYPE(),VTK_PIECES_EXTENT);
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkOOCDFieldTracer::RequestUpdateExtent(
                vtkInformation *vtkNotUsed(request),
                vtkInformationVector **inputVector,
                vtkInformationVector *outputVector)
{
  #ifdef vtkOOCDFieldTracerDEBUG
    cerr << "====================================================================RequestUpdateExtent" << endl;
  #endif

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int ghostLevel =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  // This has the effect to run the source on all procs, without it
  // only process 0 gets the source data.
  int piece=0;
  int numPieces=1;
  if (!this->UseDynamicScheduler)
    {
    piece=outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    numPieces=outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    }
  // Seed point input.
  int nSources=inputVector[1]->GetNumberOfInformationObjects();
  for (int i=0; i<nSources; ++i)
    {
    vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(i);
    if (sourceInfo)
      {
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),piece);
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),numPieces);
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),ghostLevel);
      }
    }

  // Terminator surface input. Always request all data onall procs.
  nSources=inputVector[2]->GetNumberOfInformationObjects();
  for (int i=0; i<nSources; ++i)
    {
    vtkInformation *sourceInfo = inputVector[2]->GetInformationObject(i);
    if (sourceInfo)
      {
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),0);
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),1);
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),ghostLevel);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkOOCDFieldTracer::RequestInformation(
                vtkInformation *vtkNotUsed(request),
                vtkInformationVector **vtkNotUsed(inputVector),
                vtkInformationVector *outputVector)
{
  #ifdef vtkOOCDFieldTracerDEBUG
    cerr << "====================================================================RequestInformation" << endl;
  #endif

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkOOCDFieldTracer::RequestData(
                vtkInformation *vtkNotUsed(request),
                vtkInformationVector **inputVector,
                vtkInformationVector *outputVector)
{
  #ifdef vtkOOCDFieldTracerDEBUG
  cerr << "====================================================================RequestData" << endl;
  #endif
  #if defined vtkOOCDFieldTracerTIME
  timeval wallt;
  gettimeofday(&wallt,0x0);
  double walls=(double)wallt.tv_sec+((double)wallt.tv_usec)/1.0E6;
  #endif

  int procId=this->Controller->GetLocalProcessId();
  int nProcs=this->Controller->GetNumberOfProcesses();

  vtkInformation *info;


  /// Reader
  // Get the input on the first port. This should be the dummy dataset
  // produced by a meta-reader. The information object should have the
  // OOC Reader.
  info = inputVector[0]->GetInformationObject(0);
  if (!info->Has(vtkOOCReader::READER()))
    {
    vtkWarningMacro(
        "OOCReader object not present in input pipeline information! Aborting request.");
    return 1;
    }
  vtkSmartPointer<vtkOOCReader> oocr;
  oocr=dynamic_cast<vtkOOCReader*>(info->Get(vtkOOCReader::READER()));
  if (!oocr)
    {
    vtkWarningMacro(
        "OOCReader object not initialized! Aborting request.");
    return 1;
    }
  // Configure the reader
  vtkInformation *arrayInfo=this->GetInputArrayInformation(0);
  const char *fieldName=arrayInfo->Get(vtkDataObject::FIELD_NAME());
  oocr->Register(0);
  oocr->DeActivateAllArrays();
  oocr->ActivateArray(fieldName);
  oocr->Open();
  // We provide the integrator a pointer to a cache. The integrator
  // uses this as it sees fit to reduce the number of reads. If its
  // not null after the integrator returns we have to delete.
  vtkDataSet *oocrCache=0;

  // also the bounds (problem domain) of the data should be provided by the
  // meta reader.
  if (!info->Has(vtkOOCReader::BOUNDS()))
    {
    vtkWarningMacro(
        "Bounds not found in pipeline information! Aborting request.");
    return 1;
    }
  double pDomain[6];
  info->Get(vtkOOCReader::BOUNDS(),pDomain);


  /// Seed source
  // TODO for now we consider a seed single source, but we should handle
  // multiple sources.
  info=inputVector[1]->GetInformationObject(0);
  vtkDataSet *source
    = dynamic_cast<vtkDataSet*>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (source==0)
    {
    vtkWarningMacro("Seed source input was not present. Aborting request.");
    return 1;
    }

  /// Output
  // Get the filter's output.
  info=outputVector->GetInformationObject(0);
  vtkDataSet *out
    = dynamic_cast<vtkDataSet*>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (out==0)
    {
    vtkWarningMacro("Output dataset was not present. Aborting request.");
    return 1;
    }

  /// Map
  // Configure the map.
  FieldTopologyMap *topoMap;

  // There are two distinct modes the filter can be used in. The first is topology
  // mode. Here a topology map is produced on a set of cells(source). Field lines
  // are imediately freed and not passed to the output. In the second mode (trace mode)
  // the topology map is projected onto the field lines them selves. This mode uses
  // quite a bit more memory! 
  if (this->TopologyMode)
    {
    vtkPolyData *sourcePd, *outPd;
    vtkUnstructuredGrid *sourceUg, *outUg;

    if ((sourcePd=dynamic_cast<vtkPolyData*>(source))
      && (outPd=dynamic_cast<vtkPolyData*>(out)))
      {
      topoMap=new PolyDataFieldTopologyMap;
      topoMap->SetSource(sourcePd);
      topoMap->SetOutput(outPd);
      }
    else
    if ((sourceUg=dynamic_cast<vtkUnstructuredGrid*>(source))
      && (outUg=dynamic_cast<vtkUnstructuredGrid*>(out)))
      {
      topoMap=new UnstructuredFieldTopologyMap;
      topoMap->SetSource(sourceUg);
      topoMap->SetOutput(outUg);
      }
    }
  else
    {
    topoMap=new TracerFieldTopologyMap;
    topoMap->SetSource(source);
    topoMap->SetOutput(out);
    }

  // Make sure the termination conditions include the problem
  // domain, any other termination conditions are optional.
  TerminationCondition *tcon=topoMap->GetTerminationCondition();
  tcon->SetProblemDomain(pDomain);
  // Include and additional termination surfaces from the third and
  // optional input.
  int nSurf=inputVector[2]->GetNumberOfInformationObjects();
  for (int i=0; i<nSurf; ++i)
    {
    info=inputVector[2]->GetInformationObject(i);
    vtkPolyData *pd=
    dynamic_cast<vtkPolyData*>(info->Get(vtkDataObject::DATA_OBJECT()));
    if (pd==0)
      {
      vtkWarningMacro("Termination surface is not polydata. Skipping.");
      continue;
      }
    // SciberQuest filters provide a name that is use when creating
    // the legend. (optional).
    const char *surfName=0;
    if (info->Has(vtkMetaDataKeys::DESCRIPTIVE_NAME()))
      {
      surfName=info->Get(vtkMetaDataKeys::DESCRIPTIVE_NAME());
      }
    tcon->PushSurface(pd,surfName);
    }
  tcon->InitializeColorMapper();

  /// Work loops
  if (this->UseDynamicScheduler)
    {
    #ifdef vtkOOCDFieldTracerDEBUG
    cerr << "Starting dynamic scheduler." << endl;
    #endif
    // This requires all process to have all the seed source data
    // present.

    // TODO implement a test to verify this is so. Eg.all should have the same first point
    // and number of cells.
    this->IntegrateDynamic(procId,nProcs,source,out,fieldName,oocr.GetPointer(),oocrCache,topoMap);
    }
  else
    {
    #ifdef vtkOOCDFieldTracerDEBUG
    cerr << "Static distribution assumed." << endl;
    #endif
    // This assumes that seed source is distrubuted such that each 
    // process has a unique portion of the work.

    // TODO implement a test to verify this is so. Eg.all should have the same first point
    // and number of cells.
    this->IntegrateStatic(source,out,fieldName,oocr.GetPointer(),oocrCache,topoMap);
    }

  #if defined vtkOOCDFieldTracerTIME
  gettimeofday(&wallt,0x0);
  double walle=(double)wallt.tv_sec+((double)wallt.tv_usec)/1.0E6;
  cerr << procId << " finished " << walle-walls << endl;
  #endif

  // free cache
  if (oocrCache){ oocrCache->Delete(); }

  /// Clean up
  // print a legend, and (optionally) reduce the number of colors to that which
  // are used. The reduction makes use of global communication.
  topoMap->PrintLegend(this->SqueezeColorMap);

  // close the open file and release reader.
  oocr->Close();
  oocr->Delete();

  delete topoMap;

  return 1;
}

//-----------------------------------------------------------------------------
inline
int vtkOOCDFieldTracer::IntegrateStatic(
      vtkDataSet *source,
      vtkDataSet *out,
      const char *fieldName,
      vtkOOCReader *oocr,
      vtkDataSet *&oocrCache,
      FieldTopologyMap *topoMap)
{
  // do all local ids in a single pass.
  CellIdBlock sourceIds;
  sourceIds.first()=0;
  sourceIds.size()=source->GetNumberOfCells();

  return
    this->IntegrateBlock(
            &sourceIds,
            topoMap,
            fieldName,
            oocr,
            oocrCache);
}

//-----------------------------------------------------------------------------
int vtkOOCDFieldTracer::IntegrateDynamic(
      int procId,
      int nProcs,
      vtkDataSet *source,
      vtkDataSet *out,
      const char *fieldName,
      vtkOOCReader *oocr,
      vtkDataSet *&oocrCache,
      FieldTopologyMap *topoMap)
{
  const int masterProcId=1; // NOTE: proc 0 is busy with PV overhead.
  const int BLOCK_REQ=2222;
  // Master process distributes the work and integrates
  // in between servicing requests for work.
  if (procId==masterProcId)
    {
    int nCells=source->GetNumberOfCells();
    int workerBlockSize=min(this->WorkerBlockSize,max(nCells/nProcs,1));
    int masterBlockSize=min(workerBlockSize,this->MasterBlockSize);
    WorkQueue Q(nCells);
    int nActiveWorkers=nProcs-1;
    int moreWork=1;
    while (nActiveWorkers || moreWork)
      {
      // dispatch any and all pending requests
      int pendingReq=0;
      do
        {
        MPI_Status stat;
        MPI_Iprobe(MPI_ANY_SOURCE,BLOCK_REQ,MPI_COMM_WORLD,&pendingReq,&stat);
        if (pendingReq)
          {
          // the pending message lets us know someone needs work. The
          // message itself contains no useful information.
          int buf;
          int otherProc=stat.MPI_SOURCE;
          MPI_Recv(&buf,1,MPI_INT,otherProc,BLOCK_REQ,MPI_COMM_WORLD,&stat);
          // get a block of work.
          CellIdBlock sourceIds;
          moreWork=Q.GetBlock(sourceIds,workerBlockSize);
          // send the work. If there is no more, send zero sized block
          // this closes all workers.
          MPI_Send(sourceIds.data(),sourceIds.dataSize(),MPI_INT,otherProc,BLOCK_REQ,MPI_COMM_WORLD);
          if (!moreWork)
            {
            --nActiveWorkers;
            }
           #ifdef vtkOOCDFieldTracerDEBUG
           cerr << "Master filled request from " << otherProc << " " << sourceIds << endl;
           #endif
          }
        }
      while (pendingReq);

      // now that all the worker that need work have it. Do a small amount
      // of work while the others are busy.
      CellIdBlock sourceIds;
      moreWork=Q.GetBlock(sourceIds,masterBlockSize);
      if (moreWork)
        {
        #ifdef vtkOOCDFieldTracerDEBUG
        cerr << "Master integrating " << sourceIds << endl;
        #endif
        this->IntegrateBlock(
                &sourceIds,
                topoMap,
                fieldName,
                oocr,
                oocrCache);
        }
      }
    }
  // Work processes receive chunks of seed cell ids and 
  // integrate.
  else
    {
    while (1)
      {
      #ifdef vtkOOCDFieldTracerDEBUG
      cerr << "Slave " << procId << " requesting work" << endl;
      #endif
      // get a block of seed cell ids to process.
      MPI_Send(&procId,1,MPI_INT,masterProcId,BLOCK_REQ,MPI_COMM_WORLD);
      MPI_Status stat;
      CellIdBlock sourceIds;
      MPI_Recv(sourceIds.data(),sourceIds.dataSize(),MPI_INT,masterProcId,BLOCK_REQ,MPI_COMM_WORLD,&stat);

      #ifdef vtkOOCDFieldTracerDEBUG
      cerr << "Slave " << procId << " received " << sourceIds << endl;
      #endif

      // stop when no work has been issued.
      if (sourceIds.size()==0){ break; }

      // integrate this block
      this->IntegrateBlock(
                &sourceIds,
                topoMap,
                fieldName,
                oocr,
                oocrCache);
      }
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkOOCDFieldTracer::IntegrateBlock(
      CellIdBlock *sourceIds,
      FieldTopologyMap *topoMap,
      const char *fieldName,
      vtkOOCReader *oocr,
      vtkDataSet *&oocrCache)

{
  // build the output.
  vtkIdType nLines=topoMap->InsertCells(sourceIds);

  TerminationCondition *tcon=topoMap->GetTerminationCondition();

  // integrate from each source cell.
  for (vtkIdType i=0; i<nLines; ++i)
    {
    FieldLine *line=topoMap->GetFieldLine(i);
    this->IntegrateOne(oocr,oocrCache,fieldName,line,tcon);
    if (this->TopologyMode)
      {
      // free the trace geometry, we don't need it for topo map
      // and it's very likely to be large.
      line->DeleteTrace();
      }
    }

  // sync results to output. (minimizes the calls to realloc)
  topoMap->SyncScalars();
  topoMap->SyncGeometry();

  // free resources in preparation for the next pass.
  topoMap->ClearFieldLines();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkOOCDFieldTracer::IntegrateOne(
      vtkOOCReader *oocR,
      vtkDataSet *&oocRCache,
      const char *fieldName,
      FieldLine *line,
      TerminationCondition *tcon)
{
  // we integrate twice, once forward and once backward starting
  // from the single seed point.
  for (int i=0, stepSign=-1; i<2; stepSign+=2, ++i)
    {
    double minStep=0.0;             // smallest step to take (can be a function of cell size)
    double maxStep=0.0;             // largest step to take
    double lineLength=0.0;          // length of field line
    double V0[3]={0.0};             // vector field interpolated at the start point
    double p0[3]={0.0};             // a start point
    double p1[3]={0.0};             // intergated point
    int numSteps=0;                 // number of steps taken in integration
    double stepSize=this->MaxStep;  // next recommended step size
    static
    vtkInterpolatedVelocityField *interp=0;

    line->GetSeedPoint(p0);

    if (tcon->OutsideProblemDomain(p0))
      {
      vtkWarningMacro(
        "Seed point " << p0[0] << ", " << p0[1] << ", " << p0[2]
        << " is outside of the problem domain. Seed source is expected"
        << " to be contained by the problem domain. Aborting integration.");
      return;
      }

    // Integrate until the maximum line length is reached, maximum number of 
    // steps is reached or until a termination surface is encountered.
    while (1)
      {
      // add the point to the line.
      if (!this->TopologyMode) line->PushPoint(i,p0);

      #ifdef vtkOOCDFieldTracerDEBUG5
      cerr << "   " << p0[0] << ", " << p0[1] << ", " << p0[2] << endl;
      #endif

      // We are now integrated through data in memory we need to
      // fetch another neighborhood about the seed point from disk.
      if (tcon->OutsideWorkingDomain(p0))
        {
        // Read data in the neighborhood of the seed point.
        if (oocRCache){ oocRCache->Delete(); }
        oocRCache=oocR->ReadNeighborhood(p0,this->OOCNeighborhoodSize);
        if (!oocRCache)
          {
          vtkWarningMacro("Failed to read neighborhood. Aborting.");
          return;
          }
        // update the working domain. It will be used to validate the next read.
        tcon->SetWorkingDomain(oocRCache->GetBounds());

        // Initialize the vector field interpolator.
        interp=vtkInterpolatedVelocityField::New();
        interp->AddDataSet(oocRCache);
        interp->SelectVectors(fieldName);

        this->Integrator->SetFunctionSet(interp);
        interp->Delete();
        }

      // interpolate vector field at seed point.
      interp->FunctionValues(p0,V0);
      double speed=vtkMath::Norm(V0);
      // check for field null
      if ((speed==0) || (speed<=this->NullThreshold))
        {
        #ifdef vtkOOCDFieldTracerDEBUG4
        cerr << "Terminated: Field null encountered." << endl;
        #endif
        line->SetTerminator(i,tcon->GetFieldNullId());
        break; // stop integrating
        }

      // Convert intervals from cell fractions into lengths.
      vtkCell* cell=oocRCache->GetCell(interp->GetLastCellId());
      double cellLength=sqrt(cell->GetLength2());
      this->ClipStep(stepSize,stepSign,minStep,maxStep,cellLength,lineLength);

      /// Integrate
      // Calculate the next step using the integrator provided.
      // Note, both stepSizeInterval and stepTaken are modified
      // by the rk45 integrator.
      interp->SetNormalizeVector(true);
      double error=0.0;
      double stepTaken=0.0;
      this->Integrator->ComputeNextStep(
          p0,p1,0,
          stepSize,stepTaken,
          minStep,maxStep,
          this->MaxError,
          error);
      interp->SetNormalizeVector(false);

      // Update the lineLength
      lineLength+=fabs(stepTaken);


      // check for line crossing a termination surface.
      int surfIsect=tcon->SegmentTerminates(p0,p1);
      if (surfIsect)
        {
        #ifdef vtkOOCDFieldTracerDEBUG4
        cerr << "Terminated: Surface " << surfIsect-1 << " hit." << endl;
        #endif
        line->SetTerminator(i,surfIsect);
        if (!this->TopologyMode) line->PushPoint(i,p1);
        break;
        }

      // We are now integrated through all available.
      if (tcon->OutsideProblemDomain(p1))
        {
        #ifdef vtkOOCDFieldTracerDEBUG4
        cerr << "Terminated: Integration outside problem domain." << endl;
        #endif
        line->SetTerminator(i,tcon->GetProblemDomainSurfaceId());
        if (!this->TopologyMode) line->PushPoint(i,p1);
        break; // stop integrating
        }

      // check integration limit
      if (lineLength>this->MaxLineLength || numSteps>this->MaxNumberOfSteps)
        {
        #ifdef vtkOOCDFieldTracerDEBUG4
        cerr << "Terminated: Integration limmit exceeded." << endl;
        #endif
        line->SetTerminator(i,tcon->GetShortIntegrationId());
        if (!this->TopologyMode) line->PushPoint(i,p1);
        break; // stop integrating
        }

      // Use v=dx/dt to calculate speed and check if it is below
      // stagnation threshold
      double dx=0;
      dx+=(p1[0]-p0[0])*(p1[0]-p0[0]);
      dx+=(p1[1]-p0[1])*(p1[1]-p0[1]);
      dx+=(p1[2]-p0[2])*(p1[2]-p0[2]);
      dx=sqrt(dx);
      double dt=fabs(stepTaken);
      double v=dx/(dt+1E-12);
      if (v<=this->NullThreshold)
        {
        #ifdef vtkOOCDFieldTracerDEBUG4
        cerr << "Terminated: Field null encountered." << endl;
        #endif
        line->SetTerminator(i,tcon->GetFieldNullId());
        if (!this->TopologyMode) line->PushPoint(i,p1);
        break; // stop integrating
        }

      // new start point
      p0[0]=p1[0];
      p0[1]=p1[1];
      p0[2]=p1[2];
      } /// End Integration
    } /// End fwd/backwd trace
  return;
}


//-----------------------------------------------------------------------------
double vtkOOCDFieldTracer::ConvertToLength(
      double interval,
      int unit,
      double cellLength)
{
  double retVal = 0.0;
  if (unit==ARC_LENGTH )
    {
    retVal=interval;
    }
  else
  if (unit==CELL_FRACTION)
    {
    retVal=interval*cellLength;
    }
  return retVal;
}

//-----------------------------------------------------------------------------
void vtkOOCDFieldTracer::ClipStep(
      double& step,
      int stepSign,
      double& minStep,
      double& maxStep,
      double cellLength,
      double lineLength)
{
  // clip to cell fraction
  minStep=this->ConvertToLength(this->MinStep,this->StepUnit,cellLength);
  maxStep=this->ConvertToLength(this->MaxStep,this->StepUnit,cellLength);
  if (step<minStep)
    {
    step=minStep;
    }
  else
  if (step>maxStep)
    {
    step=maxStep;
    }
  // clip to max line length
  double newLineLength=step+lineLength;
  if (newLineLength>this->MaxLineLength)
    {
    step=newLineLength-this->MaxLineLength;
    }
  // fix up the sign (this assumes that step is always > 0)
  step*=stepSign;
  minStep*=stepSign;
  maxStep*=stepSign;
  /// cerr << "Intervals: " << minStep << ", " << step << ", " << maxStep << ", " << direction << endl;
}

//-----------------------------------------------------------------------------
void vtkOOCDFieldTracer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  // TODO
}
