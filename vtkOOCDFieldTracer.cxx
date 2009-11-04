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
  StepUnit(CELL_FRACTION),
  InitialStep(0.1),
  MinStep(0.1),
  MaxStep(0.5),
  MaxError(1E-5),
  MaxNumberOfSteps(1000),
  MaxLineLength(1E6),
  TerminalSpeed(1E-6),
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
  // int piece
  //   =outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  // int numPieces
  //   =outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int ghostLevel =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  // This has the effect to run the source on all procs, without it
  // only process 0 gets the source data.

  // Seed point input.
  int nSources=inputVector[1]->GetNumberOfInformationObjects();
  for (int i=0; i<nSources; ++i)
    {
    vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(i);
    if (sourceInfo)
      {
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),0);
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),1);
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),ghostLevel);
      }
    }

  // Terminator surface input.
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
  // produced by ameta-reader. The information object should have the
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
  // cache
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
  const int BLOCK_REQ=2222;
  // Master process distributes the work and integrates
  // in between servicing requests for work.
  if (procId==0)
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
                oocr.GetPointer(),
                oocrCache);
        }
      }
    }
  // Work processes reveive chunks of seed cell ids and 
  // integrate.
  else
    {
    while (1)
      {
      #ifdef vtkOOCDFieldTracerDEBUG
      cerr << "Slave " << procId << " requesting work" << endl;
      #endif
      // get a block of seed cell ids to process.
      MPI_Send(&procId,1,MPI_INT,0,BLOCK_REQ,MPI_COMM_WORLD);
      MPI_Status stat;
      CellIdBlock sourceIds;
      MPI_Recv(sourceIds.data(),sourceIds.dataSize(),MPI_INT,0,BLOCK_REQ,MPI_COMM_WORLD,&stat);

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
                oocr.GetPointer(),
                oocrCache);
      }
    }

  // print a legend, and (optionally) reduce the number of colors to that which
  // are used. This makes use of global communication!
  topoMap->PrintLegend(this->SqueezeColorMap);

  // close the open file and release reader.
  oocr->Close();
  oocr->Delete();
  // free cache
  if (oocrCache){ oocrCache->Delete(); }

  delete topoMap;

  #if defined vtkOOCDFieldTracerTIME
  gettimeofday(&wallt,0x0);
  double walle=(double)wallt.tv_sec+((double)wallt.tv_usec)/1.0E6;
  cerr << procId << " finished " << walle-walls << endl;
  #endif

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
      if ((speed==0) || (speed<=this->TerminalSpeed))
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
      if (v<=this->TerminalSpeed)
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

//   // Description:
//   // USe the set of field lines to construct a vtk polydata set. Field line structures
//   // are deleted as theya re coppied.
//   int FieldLinesToPolydata(
//         vector<FieldLine*> &lines,
//         vtkIdType nPtsTotal,
//         vtkCellArray *OutCells,
//         vtkFloatArray *OutPts);
//   // Description:
//   // Given a set of polygons (seedSource) that is assumed duplicated across
//   // all process in the communicator, extract an equal number of polygons
//   // on each process and compute seed points at the center of each local
//   // poly. The computed points are stored in new FieldLine structures. It is
//   // the callers responsibility to delete these structures.
//   // Return 0 if an error occurs. Upon successful completion the number
//   // of seed points is returned.
//   int CellsToSeeds(
//         CellIdBlock *SourceIds,
//         vtkCellArray *SourceCells,
//         vtkFloatArray *SourcePts,
//         vector<FieldLine *> &lines);
// 
//   // Description:
//   // Given a set of polygons (seedSource) that is assumed duplicated across
//   // all process in the communicator, extract an equal number of polygons
//   // on each process and compute seed points at the center of each local
//   // poly. In addition copy the local polys (seedOut). The computed points
//   // are stored in new FieldLine structures. It is the callers responsibility
//   // to delete these structures.
//   // Return 0 if an error occurs. Upon successful completion the number
//   // of seed points is returned.
//   int CellsToSeeds(
//         CellIdBlock *SourceIds,
//         vtkCellArray *SourceCells,
//         vtkFloatArray *SourcePts,
//         vtkCellArray *OutCells,
//         vtkFloatArray *OutPts,
//         map<vtkIdType,vtkIdType> &idMap,
//         vector<FieldLine *> &lines);
//   int CellsToSeeds(
//         CellIdBlock *SourceIds,
//         vtkCellArray *SourceCells,
//         vtkFloatArray *SourcePts,
//         vtkUnsingedCharArray *SourceTypes,
//         vtkCellArray *OutCells,
//         vtkFloatArray *OutPts,
//         vtkUnsingedCharArray *OutTypes,
//         vtkIdTypeArray *OutLocs,
//         map<vtkIdType,vtkIdType> &idMap,
//         vector<FieldLine *> &lines);
//   // Description:
//   // Helper to call the right methods.
//   vtkIdType CellsToSeeds(
//         CellIdBlock *sourceIds,
//         SeedGeometryCache *seedGeom,
//         vector<FieldLine *> &lines);

// //-----------------------------------------------------------------------------
// inline
// vtkIdType vtkOOCDFieldTracer::CellsToSeeds(
//         CellIdBlock *sourceIds,
//         GeometryCache *geom,
//         vector<FieldLine *> &lines)
// {
// 
//   switch (geom->GetType())
//     {
//     case GeometryCache::POLYDATA:
//       return
//         this->CellsToSeeds(
//                 sourceIds,
//                 geom->GetSourceCells(),
//                 geom->GetSourcePoints(),
//                 geom->GetOutCells(),
//                 geom->GetOutPoints(),
//                 geom->GetIdMap(),
//                 lines);
//       break;
//     case GeometryCache::UNSTRUCTURED:
//       return
//         this->CellsToSeeds(
//                 sourceIds,
//                 geom->GetSourceCells(),
//                 geom->GetSourcePoints(),
//                 geom->GetSourceTypes(),
//                 geom->GetOutCells(),
//                 geom->GetOutPoints(),
//                 geom->GetOutTypes(),
//                 geom->GetOutLocs(),
//                 geom->GetIdMap(),
//                 lines);
//       break;
//     }
// 
//   vtkWarningMacro("Seed source type is unsupported.");
//   return 0;
// }


// //-----------------------------------------------------------------------------
// int vtkOOCDFieldTracer::CellsToSeeds(
//         CellIdBlock *SourceIds,
//         vtkCellArray *SourceCells,
//         vtkFloatArray *SourcePts,
//         vtkUnsingedCharArray *SourceTypes,
//         vtkCellArray *OutCells,
//         vtkFloatArray *OutPts,
//         vtkUnsingedCharArray *OutTypes,
//         vtkIdTypeArray *OutLocs,
//         map<vtkIdType,vtkIdType> &idMap,
//         vector<FieldLine *> &lines)
// {
//   #ifdef vtkOOCDFieldTracerDEBUG
//   cerr << "====================================================================CellsToSeeds,usg->usg" << endl;
//   #endif
//   // Get the cells.
//   // vtkCellArray *SourceCells=Source->GetCells();
//   // vtkIdType nCells=SourceCells->GetNumberOfCells();
//   // if (nCells==0)
//   //   {
//   //   vtkWarningMacro("Did not find source cells. Aborting request.");
//   //   return 0;
//   //   }
//
//   vtkIdType startCellId=SourceIds->first();
//   vtkIdType nCellsLocal=SourceIds->size();
//
//   // Cells are sequentially acccessed (not random) so explicitly skip all cells
//   // we aren't interested in.
//   SourceCells->InitTraversal();
//   for (vtkIdType i=0; i<startCellId; ++i)
//     {
//     vtkIdType n;
//     vtkIdType *ptIds;
//     SourceCells->GetNextCell(n,ptIds);
//     }
//   // For each cell asigned to us we'll get its center (this is the seed point)
//   // and build corresponding cell in the output, The output only will have
//   // the cells assigned to this process.
//   // vtkFloatArray *SourcePts
//   //   = dynamic_cast<vtkFloatArray*>(Source->GetPoints()->GetData());
//   // if (SourcePts==0)
//   //   {
//   //   vtkWarningMacro("Seed source points are not float. Aborting request.");
//   //   return 0;
//   //   }
//   float *pSourcePts=SourcePts->GetPointer(0);
// 
//   // cells
//   ///vtkFloatArray *OutPts=vtkFloatArray::New();
//   ///OutPts->SetNumberOfComponents(3);
//   // vtkFloatArray *OutPts
//   //   = dynamic_cast<vtkFloatArray*>(Out->GetPoints()->GetData());
// 
//   ///vtkIdTypeArray *OutCells=vtkIdTypeArray::New();
//   vtkIdTypeArray *OutCellPtIds=OutCells->GetData();
// 
//   unsigned char *pSourceTypes=SourceTypes->GetPointer(0);
// 
//   ///vtkUnsignedCharArray *OutTypes=vtkUnsignedCharArray::New();
//   // vtkUnsignedCharArray *OutTypes=Out->GetCellTypesArray();
//   vtkIdType endOfTypes=OutTypes->GetNumberOfTuples();
//   unsigned char *pOutTypes=OutTypes->WritePointer(endOfTypes,nCellsLocal);
// 
//   ///vtkIdTypeArray *OutLocs=vtkIdTypeArray::New();
//   // vtkIdTypeArray *OutLocs=Out->GetCellLocationsArray();
//   vtkIdType endOfLocs=OutLocs->GetNumberOfTuples();
//   vtkIdType *pOutLocs=OutLocs->WritePointer(endOfLocs,nCellsLocal);
// 
//   vtkIdType nCellIds=OutCells->GetNumberOfCells();
//   vtkIdType nOutPts=OutPoints->GetNumberOfPoints();
//   vtkIdType SourceCellId=startCellId;
// 
//   int lId=lines.size();
//   lines.resize(lId+nCellsLocal,0);
// 
//   typedef pair<map<vtkIdType,vtkIdType>::iterator,bool> MapInsert;
//   typedef pair<vtkIdType,vtkIdType> MapElement;
//   ///map<vtkIdType,vtkIdType> idMap;
// 
//   for (vtkIdType i=0; i<nCellsLocal; ++i)
//     {
//     // get the cell that belong to us.
//     vtkIdType nPtIds;
//     vtkIdType *ptIds;
//     SourceCells->GetNextCell(nPtIds,ptIds);
// 
//     // set the new cell's location
//     *pOutLocs=nCellIds;
//     ++pOutLocs;
// 
//     // copy its type.
//     *pOutTypes=pSourceTypes[SourceCellId];
//     ++pOutTypes;
// 
//     // Get location to write new cell.
//     vtkIdType *pOutCells=OutCells->WritePointer(nCellIds,nPtIds+1);
//     // update next cell write location.
//     nCellIds+=nPtIds+1;
//     // number of points in this cell
//     *pOutCells=nPtIds;
//     ++pOutCells;
// 
//     // Get location to write new point. assumes we need to copy all
//     // but this is wrong as there will be many duplicates. ignored.
//     float *pOutPts=OutPts->WritePointer(3*nOutPts,3*nPtIds);
//     // the  point we will use the center of the cell
//     double seed[3]={0.0};
//     // transfer from input to output (only what we own)
//     for (vtkIdType j=0; j<nPtIds; ++j,++pOutCells)
//       {
//       vtkIdType idx=3*ptIds[j];
//       // do we already have this point?
//       MapElement elem(ptIds[j],nOutPts);
//       MapInsert ret=idMap.insert(elem);
//       if (ret.second==true)
//         {
//         // this point hasn't previsouly been coppied
//         // copy the point.
//         pOutPts[0]=pSourcePts[idx  ];
//         pOutPts[1]=pSourcePts[idx+1];
//         pOutPts[2]=pSourcePts[idx+2];
//         pOutPts+=3;
// 
//         // insert the new point id.
//         *pOutCells=nOutPts;
//         ++nOutPts;
//         }
//       else
//         {
//         // this point has been coppied, do not add a duplicate.
//         // insert the other point id.
//         *pOutCells=(*ret.first).second;
//         }
//       // compute contribution to cell center.
//       seed[0]+=pSourcePts[idx  ];
//       seed[1]+=pSourcePts[idx+1];
//       seed[2]+=pSourcePts[idx+2];
//       }
//     // finsih the seed point computation (at cell center).
//     seed[0]/=nPtIds;
//     seed[1]/=nPtIds;
//     seed[2]/=nPtIds;
// 
//     lines[lId]=new FieldLine(seed,SourceCellId);
//     ++SourceCellId;
//     ++lId;
//     }
// 
//   // correct the length of the point array, above we assumed 
//   // that all points from each cell needed to be inserted
//   // and allocated that much space.
//   OutPts->SetNumberOfTuples(nOutPts);
// 
//   // put the new cells and points into the filter's output.
//   /// vtkPoints *OutPoints=vtkPoints::New();
//   /// OutPoints->SetData(OutPts);
//   /// Out->SetPoints(OutPoints);
//   /// OutPoints->Delete();
//   /// OutPts->Delete();
// 
//   /// vtkCellArray *OutCellA=vtkCellArray::New();
//   /// OutCellA->SetCells(nCellsLocal,OutCells);
//   /// Out->SetCells(OutTypes,OutLocs,OutCellA);
//   /// OutCellA->Delete();
//   /// OutCells->Delete();
// 
//   return nCellsLocal;
// }


// //-----------------------------------------------------------------------------
// int vtkOOCDFieldTracer::CellsToSeeds(
//         CellIdBlock *SourceIds,
//         vtkCellArray *SourceCells,
//         vtkFloatArray *SourcePts,
//         vtkCellArray *OutCells,
//         vtkFloatArray *OutPts,
//         map<vtkIdType,vtkIdType> &idMap,
//         vector<FieldLine *> &lines)
// {
//   #ifdef vtkOOCDFieldTracerDEBUG
//   cerr << "====================================================================CellsToSeeds,poly->poly" << endl;
//   #endif
// 
//   vtkIdType startCellId=SourceIds->first();
//   vtkIdType nCellsLocal=SourceIds->size();
// 
//   // update the cell count before we forget (does not allocate).
//   OutCells->SetNumberOfCells(OutCells->GetNumberOfCells()+nCellsLocal);
// 
//   // Cells are sequentially acccessed (not random) so explicitly skip all cells
//   // we aren't interested in.
//   SourceCells->InitTraversal();
//   for (vtkIdType i=0; i<startCellId; ++i)
//     {
//     vtkIdType n;
//     vtkIdType *ptIds;
//     SourceCells->GetNextCell(n,ptIds);
//     }
//   // For each cell asigned to us we'll get its center (this is the seed point)
//   // and build corresponding cell in the output, The output only will have
//   // the cells assigned to this process.
//   float *pSourcePts=SourcePts->GetPointer(0);
// 
//   // polygonal cells
//   ///vtkFloatArray *OutCellPts=vtkFloatArray::New();
//   ///OutCellPts->SetNumberOfComponents(3);
//   vtkFloatArray *OutCellPts
//     = dynamic_cast<vtkFloatArray*>(Out->GetPoints()->GetData());
// 
//   ///vtkIdTypeArray *OutCellCells=vtkIdTypeArray::New();
//   vtkIdTypeArray *OutCellCells=OutCells->GetData();
//   vtkIdType insertLoc=OutCellCells->GetNumberOfTuples();
// 
// 
//   vtkIdType nOutCellPts=Out->GetNumberOfPoints();
//   vtkIdType polyId=startCellId;
// 
//   ///lines.clear();
//   int lId=lines.size();
//   lines.resize(lId+nCellsLocal,0);
// 
//   typedef pair<map<vtkIdType,vtkIdType>::iterator,bool> MapInsert;
//   typedef pair<vtkIdType,vtkIdType> MapElement;
//   ///map<vtkIdType,vtkIdType> idMap;
// 
//   for (vtkIdType i=0; i<nCellsLocal; ++i)
//     {
//     // Get the cell that belong to us.
//     vtkIdType nPtIds;
//     vtkIdType *ptIds;
//     SourceCells->GetNextCell(nPtIds,ptIds);
// 
//     // Get location to write new cell.
//     vtkIdType *pOutCellCells=OutCellCells->WritePointer(insertLoc,nPtIds+1);
//     // update next cell write location.
//     insertLoc+=nPtIds+1;
//     // number of points in this cell
//     *pOutCellCells=nPtIds;
//     ++pOutCellCells;
// 
//     // Get location to write new point. assumes we need to copy all
//     // but this is wrong as there will be many duplicates. ignored.
//     float *pOutCellPts=OutCellPts->WritePointer(3*nOutCellPts,3*nPtIds);
//     // the  point we will use the center of the cell
//     double seed[3]={0.0};
//     // transfer from input to output (only what we own)
//     for (vtkIdType j=0; j<nPtIds; ++j,++pOutCellCells)
//       {
//       vtkIdType idx=3*ptIds[j];
//       // do we already have this point?
//       MapElement elem(ptIds[j],nOutCellPts);
//       MapInsert ret=idMap.insert(elem);
//       if (ret.second==true)
//         {
//         // this point hasn't previsouly been coppied
//         // copy the point.
//         pOutCellPts[0]=pSourcePts[idx  ];
//         pOutCellPts[1]=pSourcePts[idx+1];
//         pOutCellPts[2]=pSourcePts[idx+2];
//         pOutCellPts+=3;
// 
//         // insert the new point id.
//         *pOutCellCells=nOutCellPts;
//         ++nOutCellPts;
//         }
//       else
//         {
//         // this point has been coppied, do not add a duplicate.
//         // insert the other point id.
//         *pOutCellCells=(*ret.first).second;
//         }
//       // compute contribution to cell center.
//       seed[0]+=pSourcePts[idx  ];
//       seed[1]+=pSourcePts[idx+1];
//       seed[2]+=pSourcePts[idx+2];
//       }
//     // finsih the seed point computation (at cell center).
//     seed[0]/=nPtIds;
//     seed[1]/=nPtIds;
//     seed[2]/=nPtIds;
// 
//     lines[lId]=new FieldLine(seed,polyId);
//     ++polyId;
//     ++lId;
//     }
//   // correct the length of the point array, above we assumed 
//   // that all points from each cell needed to be inserted
//   // and allocated that much space.
//   OutCellPts->SetNumberOfTuples(nOutCellPts);
// 
//   // put the new cells and points into the filter's output.
//   /// vtkPoints *OutCellPoints=vtkPoints::New();
//   /// OutCellPoints->SetData(OutCellPts);
//   /// Out->SetPoints(OutCellPoints);
//   /// OutCellPoints->Delete();
//   /// OutCellPts->Delete();
// 
//   /// vtkCellArray *OutCells=vtkCellArray::New();
//   /// OutCells->SetCells(nCellsLocal,OutCellCells);
//   /// SetCells(Out,OutCells,SourceType);
//   /// OutCells->Delete();
//   /// OutCellCells->Delete();
// 
//   return nCellsLocal;
// }
/*
//-----------------------------------------------------------------------------
int vtkOOCDFieldTracer::CellsToSeeds(
        CellIdBlock *SourceIds,
        vtkCellArray *SourceCells,
        vtkFloatArray *SourcePts,
        vector<FieldLine *> &lines)
{
  #ifdef vtkOOCDFieldTracerDEBUG
  cerr << "====================================================================CellsToSeeds,points" << endl;
  #endif

  vtkIdType startId=SourceIds->first();
  vtkIdType endId=SourceIds->last();
  vtkIdType nCellsLocal=SourceIds->size();

  int lId=lines.size();
  lines.resize(lId+nCellsLocal,0);


  // For each cell asigned to us we'll get its center (this is the seed point)
  // and build corresponding cell in the output, The output only will have
  // the cells assigned to this process.
  float *pSourcePts=sourcePts->GetPointer(0);

  // Cells are sequentially acccessed (not random) so explicitly skip all cells
  // we aren't interested in.
  SourceCells->InitTraversal();
  for (vtkIdType i=0; i<startCellId; ++i)
    {
    vtkIdType n;
    vtkIdType *ptIds;
    SourceCells->GetNextCell(n,ptIds);
    }

  vtkIdList *ptIds=vtkIdList::New();
  for (vtkIdType cId=startId; cId<endId; ++cId)
    {
    // get the cell that belong to us.
    vtkIdType nPtIds;
    vtkIdType *ptIds;
    SourceCells->GetNextCell(nPtIds,ptIds);

    // the seed point we will use the center of the cell
    double seed[3]={0.0};
    // transfer from input to output (only what we own)
    for (vtkIdType pId=0; pId<nPtIds; ++pId)
      {
      vtkIdType idx=3*ptIds[pId];
      // compute contribution to cell center.
      seed[0]+=pSourcePts[idx  ];
      seed[1]+=pSourcePts[idx+1];
      seed[2]+=pSourcePts[idx+2];
      }
    // finsih the seed point computation (at cell center).
    seed[0]/=nPtIds;
    seed[1]/=nPtIds;
    seed[2]/=nPtIds;

    lines[lId]=new FieldLine(seed,cId);
    ++lId;
    }
  ptIds->Delete();

  return nCellsLocal;
}*/

// //-----------------------------------------------------------------------------
// int vtkOOCDFieldTracer::FieldLinesToPolydata(
//         vector<FieldLine *> &lines,
//         vtkIdType nPtsTotal,
//         vtkCellArray *OutCells,
//         vtkFloatArray *OutPts)
// {
//   #ifdef vtkOOCDFieldTracerDEBUG
//   cerr << "====================================================================FieldLinesToPolydata" << endl;
//   #endif
// 
//   // vtkPolyData *Out=dynamic_cast<vtkPolyData*>(out);
//   // if (!Out)
//   //   {
//   //   vtkWarningMacro("Output set to " << out->GetClassName() << ", but should be vtkPolyData.");
//   //   return 0;
//   //   }
// 
//   size_t nLines=lines.size();
//   // construct lines from the integrations.
//   ///vtkFloatArray *linePts=vtkFloatArray::New();
//   ///linePts->SetNumberOfComponents(3);
//   ///linePts->SetNumberOfTuples(nPtsTotal);
//   ///float *pLinePts=linePts->GetPointer(0);
//   // vtkFloatArray *linePts=dynamic_cast<vtkFloatArray*>(Out->GetPoints()->GetData());
//   vtkIdType nOutPts=OutPts->GetNumberOfTuples();
//   float *pOutPts=OutPts->WritePointer(3*nOutPts,3*nPtsTotal);
// 
// 
//   ///vtkIdTypeArray *lineCells=vtkIdTypeArray::New();
//   ///lineCells->SetNumberOfTuples(nPtsTotal+nLines);
//   ///vtkIdType *pLineCells=lineCells->GetPointer(0);
//   vtkIdTypeArray *lineCells=OutCells->GetData();
//   vtkIdType nLineCells=Out->OutCells->GetNumberOfCells();
//   vtkIdType *pLineCells=lineCells->WritePointer(nLineCells,nPtsTotal+nLines);
// 
//   vtkIdType ptId=nLinePts;
// 
//   for (size_t i=0; i<nLines; ++i)
//     {
//     // copy the points
//     vtkIdType nLinePts=lines[i]->CopyPointsTo(pLinePts);
//     pLinePts+=3*nLinePts;
// 
//     // build the cell
//     *pLineCells=nLinePts;
//     ++pLineCells;
//     for (vtkIdType q=0; q<nLinePts; ++q)
//       {
//       *pLineCells=ptId;
//       ++pLineCells;
//       ++ptId;
//       }
//     delete lines[i];
//     }
// 
//   lines.clear();
// 
//   // points
//   ///vtkPoints *fieldLinePoints=vtkPoints::New();
//   ///fieldLinePoints->SetData(linePts);
//   ///fieldLines->SetPoints(fieldLinePoints);
//   ///fieldLinePoints->Delete();
//   ///linePts->Delete();
//   // cells
//   ///vtkCellArray *fieldLineCells=vtkCellArray::New();
//   ///fieldLineCells->SetCells(nLines,lineCells);
//   ///fieldLines->SetLines(fieldLineCells);
//   ///fieldLineCells->Delete();
//   ///lineCells->Delete();
//   ///#ifdef vtkOOCDFieldTracerDEBUG
//   // cell data
//   ///fieldLines->GetCellData()->AddArray(lineIds);
//   ///lineIds->Delete();
//   ///#endif
// 
//   return 1;
// }