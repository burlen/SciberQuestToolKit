/*=========================================================================

Program:   Visualization Toolkit
Module:    $RCSfile: vtkOOCFieldTracer.cxx,v $

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOOCFieldTracer.h"
// #define vtkOOCFieldTracerDEBUG


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
//#include "vtkCompositeDataIterator.h"
//#include "vtkCompositeDataPipeline.h"
//#include "vtkCompositeDataSet.h"
//#include "vtkDataSetAttributes.h"
//#include "vtkExecutive.h"
//#include "vtkGenericCell.h"

//#include "vtkMultiBlockDataSet.h"

#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

//#include "vtkPolyLine.h"
//#include "vtkRungeKutta2.h"
//#include "vtkRungeKutta4.h"
#include "vtkInitialValueProblemSolver.h"
#include "vtkRungeKutta45.h"
#include "vtkMultiProcessController.h"
//#include "vtkSmartPointer.h"
#include "vtkOOCReader.h"
#include "vtkMetaDataKeys.h"
#include "vtkMath.h"

#if defined PV_3_4_BUILD
  #include "vtkInterpolatedVelocityField-3.7.h"
#else
  #include "vtkInterpolatedVelocityField.h"
#endif

#include<map>
using std::map;
using std::pair;

#include "FieldLine.h"
#include "TerminationCondition.h"

vtkCxxRevisionMacro(vtkOOCFieldTracer, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkOOCFieldTracer);

const double vtkOOCFieldTracer::EPSILON = 1.0E-12;

//-----------------------------------------------------------------------------
vtkOOCFieldTracer::vtkOOCFieldTracer()
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
  this->Controller=vtkMultiProcessController::GetGlobalController();
  this->SetNumberOfInputPorts(3);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkOOCFieldTracer::~vtkOOCFieldTracer()
{
  this->Integrator->Delete();
}

//-----------------------------------------------------------------------------
int vtkOOCFieldTracer::FillInputPortInformation(int port, vtkInformation *info)
{
  #if defined vtkOOCFieldTracerDEBUG
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
int vtkOOCFieldTracer::FillOutputPortInformation(int port, vtkInformation *info)
{
  #if defined vtkOOCFieldTracerDEBUG
  cerr << "====================================================================FillOutputPortInformation" << endl;
  #endif

  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
  return 1;

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
void vtkOOCFieldTracer::AddVectorInputConnection(
                vtkAlgorithmOutput* algOutput)
{
  #if defined vtkOOCFieldTracerDEBUG
  cerr << "====================================================================AddDatasetInputConnectiont" << endl;
  #endif

  this->AddInputConnection(0, algOutput);
}

//----------------------------------------------------------------------------
void vtkOOCFieldTracer::ClearVectorInputConnections()
{
  #if defined vtkOOCFieldTracerDEBUG
  cerr << "====================================================================ClearDatasetInputConnections" << endl;
  #endif

  this->SetInputConnection(0, 0);
}

//----------------------------------------------------------------------------
void vtkOOCFieldTracer::AddSeedPointInputConnection(
                vtkAlgorithmOutput* algOutput)
{
  #if defined vtkOOCFieldTracerDEBUG
  cerr << "====================================================================AddSeedPointInputConnection" << endl;
  #endif
  this->AddInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void vtkOOCFieldTracer::ClearSeedPointInputConnections()
{
  #if defined vtkOOCFieldTracerDEBUG
  cerr << "====================================================================ClearSeedPointInputConnections" << endl;
  #endif
  this->SetInputConnection(1, 0);
}

//----------------------------------------------------------------------------
void vtkOOCFieldTracer::AddTerminatorInputConnection(
                vtkAlgorithmOutput* algOutput)
{
  #if defined vtkOOCFieldTracerDEBUG
  cerr << "====================================================================AddBoundaryInputConnection" << endl;
  #endif
  this->AddInputConnection(2, algOutput);
}

//----------------------------------------------------------------------------
void vtkOOCFieldTracer::ClearTerminatorInputConnections()
{
  #if defined vtkOOCFieldTracerDEBUG
  cerr << "====================================================================ClearBoundaryInputConnections" << endl;
  #endif
  this->SetInputConnection(2, 0);
}

//-----------------------------------------------------------------------------
void vtkOOCFieldTracer::SetStepUnit(int unit)
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
int vtkOOCFieldTracer::RequestDataObject(
                vtkInformation *info,
                vtkInformationVector** inInfos,
                vtkInformationVector* outInfos)
{
  #if defined vtkOOCFieldTracerDEBUG
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
int vtkOOCFieldTracer::RequestUpdateExtent(
                vtkInformation *vtkNotUsed(request),
                vtkInformationVector **inputVector,
                vtkInformationVector *outputVector)
{
  #if defined vtkOOCFieldTracerDEBUG
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
int vtkOOCFieldTracer::RequestInformation(
                vtkInformation *vtkNotUsed(request),
                vtkInformationVector **vtkNotUsed(inputVector),
                vtkInformationVector *outputVector)
{
  #if defined vtkOOCFieldTracerDEBUG
    cerr << "====================================================================RequestInformation" << endl;
  #endif

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkOOCFieldTracer::RequestData(
                vtkInformation *vtkNotUsed(request),
                vtkInformationVector **inputVector,
                vtkInformationVector *outputVector)
{
  #if defined vtkOOCFieldTracerDEBUG
  cerr << "====================================================================RequestData" << endl;
  #endif

  int procId=this->Controller->GetLocalProcessId();
  int nProcs=this->Controller->GetNumberOfProcesses();

  vtkInformation *info;
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
  // Configure the reader (TODO should I make a copy?)
  vtkInformation *arrayInfo=this->GetInputArrayInformation(0);
  const char *fieldName=arrayInfo->Get(vtkDataObject::FIELD_NAME());
  oocr->Register(0);
  oocr->DeActivateAllArrays();
  oocr->ActivateArray(fieldName);
  oocr->Open();

  // also the bounds (problem domain) of the data should be available.
  if (!info->Has(vtkOOCReader::BOUNDS()))
    {
    vtkWarningMacro(
        "Bounds not found in pipeline information! Aborting request.");
    return 1;
    }
  double pDomain[6];
  info->Get(vtkOOCReader::BOUNDS(),pDomain);

  // Make sure the termionation conditions include the problem
  // domain, any other termination conditions are optional.
  TerminationCondition tcon;
  tcon.SetProblemDomain(pDomain);
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
    const char *surfName=0;
    if (info->Has(vtkMetaDataKeys::DESCRIPTIVE_NAME()))
      {
      surfName=info->Get(vtkMetaDataKeys::DESCRIPTIVE_NAME());
      }
    tcon.PushSurface(pd,surfName);
    }
  tcon.InitializeColorMapper();


  // TODO for now we consider a seed single source, but we should handle
  // multiple sources.
  info=inputVector[1]->GetInformationObject(0);
  vtkDataSet *seedSource
    = dynamic_cast<vtkDataSet*>(info->Get(vtkDataObject::DATA_OBJECT()));


  // Get the filter's output.
  info=outputVector->GetInformationObject(0);
  vtkPolyData *output
    = dynamic_cast<vtkPolyData*>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (output==0)
    {
    vtkWarningMacro("Output polydata was not present. Aborting request.");
    return 1;
    }

  // cache
  vtkDataSet *cache=0;
  // internal point set for the generated lines
  vector<FieldLine *> lines;
  // intersect colors 
  vtkIntArray *intersectColor=vtkIntArray::New();
  intersectColor->SetName("IntersectColor");


  // There are two modes of operation, the traditional
  // FieldLine mode and a Topology mode. In FieldLine mode the output contains
  // field lines. In Topology mode the output contains seed cells colored by
  // where the field line terminated.
  if (this->TopologyMode)
    {
    FieldLine line;
    // compute seed points (centered on cells of input). Copy the cells
    // on which we operate into the output.
    vtkIdType nLines
      =this->DispatchCellsToSeeds(procId,nProcs,seedSource,output,lines);

    intersectColor->SetNumberOfTuples(nLines);
    int *pColor=intersectColor->GetPointer(0);

    // integrate and build surface intersection color array.
    double progInc=0.8/(double)nLines;
    for (vtkIdType i=0; i<nLines; ++i)
      {
      FieldLine *line=lines[i];
      this->UpdateProgress(i*progInc+0.10); cerr << "."; 
      this->OOCIntegrateOne(oocr,fieldName,line,&tcon,cache);
      pColor[i]=tcon.GetTerminationColor(line);
      delete line;
      lines[i]=0;
      }
    }
  else
    {
    vtkPointSet *seedSourcePs=dynamic_cast<vtkPointSet*>(seedSource);
    if (seedSourcePs==0)
      {
      vtkWarningMacro("Seed point source is not a point set. Aborting request.");
      return 1;
      }
    // compute seed points.
    vtkIdType nLines=this->CellsToSeeds(procId,nProcs,seedSourcePs,lines);

    intersectColor->SetNumberOfTuples(nLines);
    int *pColor=intersectColor->GetPointer(0);

    // integrate
    vtkIdType nPtsTotal=0;
    double progInc=0.8/(double)nLines;
    for (vtkIdType i=0; i<nLines; ++i)
      {
      FieldLine *line=lines[i];
      this->UpdateProgress(i*progInc+0.10); cerr << ".";
      this->OOCIntegrateOne(oocr,fieldName,line,&tcon,cache);
      nPtsTotal+=line->GetNumberOfPoints();
      pColor[i]=tcon.GetTerminationColor(line);
      }
    // copy into output (also deletes the lines).
    this->FieldLinesToPolydata(lines,nPtsTotal,output);
    }

  // place the color color into the polygonal cell output.
  if (this->SqueezeColorMap)
    {
    tcon.SqueezeColorMap(intersectColor);
    }
  else
    {
    tcon.PrintColorMap();
    }
  output->GetCellData()->AddArray(intersectColor);
  intersectColor->Delete();

  oocr->Close();
  oocr->Delete();

  // free cache
  if (cache){ cache->Delete(); }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkOOCFieldTracer::OOCIntegrateOne(
      vtkOOCReader *oocR,
      const char *fieldName,
      FieldLine *line,
      TerminationCondition *tcon,
      vtkDataSet *&nhood)
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

      #if defined vtkOOCFieldTracerDEBUG5
      cerr << "   " << p0[0] << ", " << p0[1] << ", " << p0[2] << endl;
      #endif

      // We are now integrated through data in memory we need to
      // fetch another neighborhood about the seed point from disk.
      if (tcon->OutsideWorkingDomain(p0))
        {
        // Read data in the neighborhood of the seed point.
        if (nhood){ nhood->Delete(); }
        nhood=oocR->ReadNeighborhood(p0,this->OOCNeighborhoodSize);
        if (!nhood)
          {
          vtkWarningMacro("Failed to read neighborhood. Aborting.");
          return;
          }
        // update the working domain. It will be used to validate the next read.
        tcon->SetWorkingDomain(nhood->GetBounds());

        // Initialize the vector field interpolator.
        interp=vtkInterpolatedVelocityField::New();
        interp->AddDataSet(nhood);
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
        #if defined vtkOOCFieldTracerDEBUG4
        cerr << "Terminated: Field null encountered." << endl;
        #endif
        line->SetTerminator(i,tcon->GetFieldNullId());
        break; // stop integrating
        }

      // Convert intervals from cell fractions into lengths.
      vtkCell* cell=nhood->GetCell(interp->GetLastCellId());
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
        #if defined vtkOOCFieldTracerDEBUG4
        cerr << "Terminated: Surface " << surfIsect-1 << " hit." << endl;
        #endif
        line->SetTerminator(i,surfIsect);
        if (!this->TopologyMode) line->PushPoint(i,p1);
        break;
        }

      // We are now integrated through all available.
      if (tcon->OutsideProblemDomain(p1))
        {
        #if defined vtkOOCFieldTracerDEBUG4
        cerr << "Terminated: Integration outside problem domain." << endl;
        #endif
        line->SetTerminator(i,tcon->GetProblemDomainSurfaceId());
        if (!this->TopologyMode) line->PushPoint(i,p1);
        break; // stop integrating
        }

      // check integration limit
      if (lineLength>this->MaxLineLength || numSteps>this->MaxNumberOfSteps)
        {
        #if defined vtkOOCFieldTracerDEBUG4
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
        #if defined vtkOOCFieldTracerDEBUG4
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
inline
vtkIdType vtkOOCFieldTracer::DispatchCellsToSeeds(
        int procId,
        int nProcs,
        vtkDataSet *Source,
        vtkDataSet *Out,
        vector<FieldLine *> &lines)
{

  vtkPolyData *inPd,*outPd;
  vtkUnstructuredGrid *inUg,*outUg;
  if ((inPd=dynamic_cast<vtkPolyData*>(Source))
    && (outPd=dynamic_cast<vtkPolyData*>(Out)))
    {
    return this->CellsToSeeds(procId,nProcs,inPd,outPd,lines);
    }
  else
  if ((inUg=dynamic_cast<vtkUnstructuredGrid*>(Source))
    && (outUg=dynamic_cast<vtkUnstructuredGrid*>(Out)))
    {
    return this->CellsToSeeds(procId,nProcs,inUg,outUg,lines);
    }


  vtkWarningMacro(
    "Seed source type " << Source->GetClassName() << " is unsupported.");
  return 0;
}


//*****************************************************************************
int GetCells(vtkPolyData *pd, vtkCellArray *&cells, int &type)
{
  if (pd->GetNumberOfPolys())
    {
    type=VTK_POLYGON;
    cells=pd->GetPolys();
    }
  else
  if (pd->GetNumberOfVerts())
    {
    type=VTK_VERTEX;
    cells=pd->GetVerts();
    }
  else
    {
    return 0;
    }
  return 1;
}

//*****************************************************************************
void SetCells(vtkPolyData *pd, vtkCellArray *cells, int type)
{
  switch (type)
    {
    case VTK_POLYGON:
      pd->SetPolys(cells);
      break;
    case VTK_VERTEX:
      pd->SetVerts(cells);
      break;
    }
}

//-----------------------------------------------------------------------------
int vtkOOCFieldTracer::CellsToSeeds(
        int procId,
        int nProcs,
        vtkPolyData *Source,
        vtkPolyData *Out,
        vector<FieldLine *> &lines)
{
  #if defined vtkOOCFieldTracerDEBUG
  cerr << "====================================================================CellsToSeeds,poly->poly" << endl;
  #endif
  // Get the cells.
  int SourceType;
  vtkCellArray *SourceCells=0;
  if (!GetCells(Source,SourceCells,SourceType))
    {
    vtkWarningMacro("Did not find supported cell type in seed source. Aborting request.");
    return 0;
    }

  // Here we break up the work load. This assumes that the seed source is duplicated
  // on all processes. RequestInformation is configured to make this happen.
  vtkIdType nCells=SourceCells->GetNumberOfCells();
  vtkIdType nCellsPerProc=nCells/nProcs;
  vtkIdType nCellsLeft=nCells%nProcs;
  vtkIdType nCellsLocal=nCellsPerProc+(procId<nCellsLeft?1:0);
  vtkIdType startCellId=procId<nCellsLeft?procId*nCellsLocal:procId*nCellsLocal+nCellsLeft;

  // Cells are sequentially acccessed (not random) so explicitly skip all cells
  // we aren't interested in.
  SourceCells->InitTraversal();
  for (vtkIdType i=0; i<startCellId; ++i)
    {
    vtkIdType n;
    vtkIdType *ptIds;
    SourceCells->GetNextCell(n,ptIds);
    }
  // For each cell asigned to us we'll get its center (this is the seed point)
  // and build corresponding cell in the output, The output only will have
  // the cells assigned to this process.
  vtkFloatArray *SourcePts
    = dynamic_cast<vtkFloatArray*>(Source->GetPoints()->GetData());
  if (SourcePts==0)
    {
    vtkWarningMacro("Seed source points are not float. Aborting request.");
    return 0;
    }
  float *pSourcePts=SourcePts->GetPointer(0);

  // polygonal cells
  vtkFloatArray *OutCellPts=vtkFloatArray::New();
  OutCellPts->SetNumberOfComponents(3);
  vtkIdTypeArray *OutCellCells=vtkIdTypeArray::New();
  vtkIdType nCellIds=0;
  vtkIdType nOutCellPts=0;
  vtkIdType polyId=startCellId;

  lines.clear();
  lines.resize(nCellsLocal,0);

  typedef pair<map<vtkIdType,vtkIdType>::iterator,bool> MapInsert;
  typedef pair<vtkIdType,vtkIdType> MapElement;
  map<vtkIdType,vtkIdType> idMap;

  for (vtkIdType i=0; i<nCellsLocal; ++i)
    {
    // Get the cell that belong to us.
    vtkIdType nPtIds;
    vtkIdType *ptIds;
    SourceCells->GetNextCell(nPtIds,ptIds);

    // Get location to write new cell.
    vtkIdType *pOutCellCells=OutCellCells->WritePointer(nCellIds,nPtIds+1);
    // update next cell write location.
    nCellIds+=nPtIds+1;
    // number of points in this cell
    *pOutCellCells=nPtIds;
    ++pOutCellCells;

    // Get location to write new point. assumes we need to copy all
    // but this is wrong as there will be many duplicates. ignored.
    float *pOutCellPts=OutCellPts->WritePointer(3*nOutCellPts,3*nPtIds);
    // the  point we will use the center of the cell
    double seed[3]={0.0};
    // transfer from input to output (only what we own)
    for (vtkIdType j=0; j<nPtIds; ++j,++pOutCellCells)
      {
      vtkIdType idx=3*ptIds[j];
      // do we already have this point?
      MapElement elem(ptIds[j],nOutCellPts);
      MapInsert ret=idMap.insert(elem);
      if (ret.second==true)
        {
        // this point hasn't previsouly been coppied
        // copy the point.
        pOutCellPts[0]=pSourcePts[idx  ];
        pOutCellPts[1]=pSourcePts[idx+1];
        pOutCellPts[2]=pSourcePts[idx+2];
        pOutCellPts+=3;

        // insert the new point id.
        *pOutCellCells=nOutCellPts;
        ++nOutCellPts;
        }
      else
        {
        // this point has been coppied, do not add a duplicate.
        // insert the other point id.
        *pOutCellCells=(*ret.first).second;
        }
      // compute contribution to cell center.
      seed[0]+=pSourcePts[idx  ];
      seed[1]+=pSourcePts[idx+1];
      seed[2]+=pSourcePts[idx+2];
      }
    // finsih the seed point computation (at cell center).
    seed[0]/=nPtIds;
    seed[1]/=nPtIds;
    seed[2]/=nPtIds;

    lines[i]=new FieldLine(seed,polyId);
    ++polyId;
    }

  // correct the length of the point array, above we assumed 
  // that all points from each cell needed to be inserted
  // and allocated that much space.
  OutCellPts->SetNumberOfTuples(nOutCellPts);

  // put the new cells and points into the filter's output.
  vtkPoints *OutCellPoints=vtkPoints::New();
  OutCellPoints->SetData(OutCellPts);
  Out->SetPoints(OutCellPoints);
  OutCellPoints->Delete();
  OutCellPts->Delete();

  vtkCellArray *OutCells=vtkCellArray::New();
  OutCells->SetCells(nCellsLocal,OutCellCells);
  SetCells(Out,OutCells,SourceType);
  OutCells->Delete();
  OutCellCells->Delete();

  return nCellsLocal;
}

//-----------------------------------------------------------------------------
int vtkOOCFieldTracer::CellsToSeeds(
        int procId,
        int nProcs,
        vtkUnstructuredGrid *Source,
        vtkUnstructuredGrid *Out,
        vector<FieldLine *> &lines)
{
  #if defined vtkOOCFieldTracerDEBUG
  cerr << "====================================================================CellsToSeeds,usg->usg" << endl;
  #endif
  // Get the cells.
  vtkCellArray *SourceCells=Source->GetCells();
  vtkIdType nCells=SourceCells->GetNumberOfCells();
  if (nCells==0)
    {
    vtkWarningMacro("Did not find source cells. Aborting request.");
    return 0;
    }

  // Here we break up the work load. This assumes that the seed source is duplicated
  // on all processes. RequestInformation is configured to make this happen.
  vtkIdType nCellsPerProc=nCells/nProcs;
  vtkIdType nCellsLeft=nCells%nProcs;
  vtkIdType nCellsLocal=nCellsPerProc+(procId<nCellsLeft?1:0);
  vtkIdType startCellId=procId<nCellsLeft?procId*nCellsLocal:procId*nCellsLocal+nCellsLeft;

  // Cells are sequentially acccessed (not random) so explicitly skip all cells
  // we aren't interested in.
  SourceCells->InitTraversal();
  for (vtkIdType i=0; i<startCellId; ++i)
    {
    vtkIdType n;
    vtkIdType *ptIds;
    SourceCells->GetNextCell(n,ptIds);
    }
  // For each cell asigned to us we'll get its center (this is the seed point)
  // and build corresponding cell in the output, The output only will have
  // the cells assigned to this process.
  vtkFloatArray *SourcePts
    = dynamic_cast<vtkFloatArray*>(Source->GetPoints()->GetData());
  if (SourcePts==0)
    {
    vtkWarningMacro("Seed source points are not float. Aborting request.");
    return 0;
    }
  float *pSourcePts=SourcePts->GetPointer(0);

  // cells
  vtkFloatArray *OutPts=vtkFloatArray::New();
  OutPts->SetNumberOfComponents(3);
  vtkIdTypeArray *OutCells=vtkIdTypeArray::New();
  unsigned char *pSourceTypes=Source->GetCellTypesArray()->GetPointer(0);
  vtkUnsignedCharArray *OutTypes=vtkUnsignedCharArray::New();
  unsigned char *pOutTypes=OutTypes->WritePointer(0,nCellsLocal);
  vtkIdTypeArray *OutLocs=vtkIdTypeArray::New();
  vtkIdType *pOutLocs=OutLocs->WritePointer(0,nCellsLocal);
  vtkIdType nCellIds=0;
  vtkIdType nOutPts=0;
  vtkIdType SourceCellId=startCellId;

  lines.clear();
  lines.resize(nCellsLocal,0);

  typedef pair<map<vtkIdType,vtkIdType>::iterator,bool> MapInsert;
  typedef pair<vtkIdType,vtkIdType> MapElement;
  map<vtkIdType,vtkIdType> idMap;

  for (vtkIdType i=0; i<nCellsLocal; ++i)
    {
    // get the cell that belong to us.
    vtkIdType nPtIds;
    vtkIdType *ptIds;
    SourceCells->GetNextCell(nPtIds,ptIds);

    // set the new cell's location
    *pOutLocs=nCellIds;
    ++pOutLocs;

    // copy its type.
    *pOutTypes=pSourceTypes[SourceCellId];
    ++pOutTypes;

    // Get location to write new cell.
    vtkIdType *pOutCells=OutCells->WritePointer(nCellIds,nPtIds+1);
    // update next cell write location.
    nCellIds+=nPtIds+1;
    // number of points in this cell
    *pOutCells=nPtIds;
    ++pOutCells;

    // Get location to write new point. assumes we need to copy all
    // but this is wrong as there will be many duplicates. ignored.
    float *pOutPts=OutPts->WritePointer(3*nOutPts,3*nPtIds);
    // the  point we will use the center of the cell
    double seed[3]={0.0};
    // transfer from input to output (only what we own)
    for (vtkIdType j=0; j<nPtIds; ++j,++pOutCells)
      {
      vtkIdType idx=3*ptIds[j];
      // do we already have this point?
      MapElement elem(ptIds[j],nOutPts);
      MapInsert ret=idMap.insert(elem);
      if (ret.second==true)
        {
        // this point hasn't previsouly been coppied
        // copy the point.
        pOutPts[0]=pSourcePts[idx  ];
        pOutPts[1]=pSourcePts[idx+1];
        pOutPts[2]=pSourcePts[idx+2];
        pOutPts+=3;

        // insert the new point id.
        *pOutCells=nOutPts;
        ++nOutPts;
        }
      else
        {
        // this point has been coppied, do not add a duplicate.
        // insert the other point id.
        *pOutCells=(*ret.first).second;
        }
      // compute contribution to cell center.
      seed[0]+=pSourcePts[idx  ];
      seed[1]+=pSourcePts[idx+1];
      seed[2]+=pSourcePts[idx+2];
      }
    // finsih the seed point computation (at cell center).
    seed[0]/=nPtIds;
    seed[1]/=nPtIds;
    seed[2]/=nPtIds;

    lines[i]=new FieldLine(seed,SourceCellId);
    ++SourceCellId;
    }

  // correct the length of the point array, above we assumed 
  // that all points from each cell needed to be inserted
  // and allocated that much space.
  OutPts->SetNumberOfTuples(nOutPts);

  // put the new cells and points into the filter's output.
  vtkPoints *OutPoints=vtkPoints::New();
  OutPoints->SetData(OutPts);
  Out->SetPoints(OutPoints);
  OutPoints->Delete();
  OutPts->Delete();

  vtkCellArray *OutCellA=vtkCellArray::New();
  OutCellA->SetCells(nCellsLocal,OutCells);
  Out->SetCells(OutTypes,OutLocs,OutCellA);
  OutCellA->Delete();
  OutCells->Delete();

  return nCellsLocal;
}

//-----------------------------------------------------------------------------
int vtkOOCFieldTracer::CellsToSeeds(
        int procId,
        int nProcs,
        vtkPointSet *Source,
        vector<FieldLine *> &lines)
{
  #if defined vtkOOCFieldTracerDEBUG
  cerr << "====================================================================CellsToSeeds,points" << endl;
  #endif
  // Here we break up the work load. This assumes that the seed source is duplicated
  // on all processes. RequestInformation is configured to make this happen.
  vtkIdType nCells=Source->GetNumberOfCells();
  if (nCells==0)
    {
    vtkWarningMacro("Did not find any cells in the seed source. Aborting request.");
    return 0;
    }
  vtkIdType nCellsPerProc=nCells/nProcs;
  vtkIdType nCellsLeft=nCells%nProcs;
  vtkIdType nCellsLocal=nCellsPerProc+(procId<nCellsLeft?1:0);
  vtkIdType startId=procId<nCellsLeft?procId*nCellsLocal:procId*nCellsLocal+nCellsLeft;
  vtkIdType endId=startId+nCellsLocal;

  lines.resize(nCellsLocal,0);

  // For each cell asigned to us we'll get its center (this is the seed point)
  // and build corresponding cell in the output, The output only will have
  // the cells assigned to this process.
  vtkFloatArray *sourcePts
    = dynamic_cast<vtkFloatArray*>(Source->GetPoints()->GetData());
  if (sourcePts==0)
    {
    vtkWarningMacro("Seed source points are not float. Aborting request.");
    return 0;
    }
  float *pSourcePts=sourcePts->GetPointer(0);

  vtkIdList *ptIds=vtkIdList::New();
  for (vtkIdType cId=startId,lId=0; cId<endId; ++cId,++lId)
    {
    // get the point ids for this cell.
    Source->GetCellPoints(cId,ptIds);
    vtkIdType nPtIds=ptIds->GetNumberOfIds();
    vtkIdType *pPtIds=ptIds->GetPointer(0);
    // the seed point we will use the center of the cell
    double seed[3]={0.0};
    // transfer from input to output (only what we own)
    for (vtkIdType pId=0; pId<nPtIds; ++pId)
      {
      vtkIdType idx=3*pPtIds[pId];
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
    }
  ptIds->Delete();

  return nCellsLocal;
}

//-----------------------------------------------------------------------------
int vtkOOCFieldTracer::FieldLinesToPolydata(
        vector<FieldLine *> &lines,
        vtkIdType nPtsTotal,
        vtkPolyData *fieldLines)
{
  #if defined vtkOOCFieldTracerDEBUG
  cerr << "====================================================================FieldLinesToPolydata" << endl;
  #endif
  size_t nLines=lines.size();
  // construct lines from the integrations.
  vtkFloatArray *linePts=vtkFloatArray::New();
  linePts->SetNumberOfComponents(3);
  linePts->SetNumberOfTuples(nPtsTotal);
  float *pLinePts=linePts->GetPointer(0);
  vtkIdTypeArray *lineCells=vtkIdTypeArray::New();
  lineCells->SetNumberOfTuples(nPtsTotal+nLines);
  vtkIdType *pLineCells=lineCells->GetPointer(0);
  vtkIdType ptId=0;
  ///#if defined vtkOOCFieldTracerDEBUG
  // cell data
  vtkFloatArray *lineIds=vtkFloatArray::New();
  lineIds->SetName("line id");
  lineIds->SetNumberOfTuples(nLines);
  float *pLineIds=lineIds->GetPointer(0);
  ///#endif
  for (size_t i=0; i<nLines; ++i)
    {
    // copy the points
    vtkIdType nLinePts=lines[i]->CopyPointsTo(pLinePts);
    pLinePts+=3*nLinePts;

    // build the cell
    *pLineCells=nLinePts;
    ++pLineCells;
    for (vtkIdType q=0; q<nLinePts; ++q)
      {
      *pLineCells=ptId;
      ++pLineCells;
      ++ptId;
      }
    ///#if defined vtkOOCFieldTracerDEBUG
    // build the cell data
    *pLineIds=lines[i]->GetSeedId();
    ++pLineIds;
    ///#endif

    delete lines[i];
    }
  // points
  vtkPoints *fieldLinePoints=vtkPoints::New();
  fieldLinePoints->SetData(linePts);
  fieldLines->SetPoints(fieldLinePoints);
  fieldLinePoints->Delete();
  linePts->Delete();
  // cells
  vtkCellArray *fieldLineCells=vtkCellArray::New();
  fieldLineCells->SetCells(nLines,lineCells);
  fieldLines->SetLines(fieldLineCells);
  fieldLineCells->Delete();
  lineCells->Delete();
  ///#if defined vtkOOCFieldTracerDEBUG
  // cell data
  fieldLines->GetCellData()->AddArray(lineIds);
  lineIds->Delete();
  ///#endif

  return 1;
}

//-----------------------------------------------------------------------------
double vtkOOCFieldTracer::ConvertToLength(
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
void vtkOOCFieldTracer::ClipStep(
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
void vtkOOCFieldTracer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  // TODO
}
