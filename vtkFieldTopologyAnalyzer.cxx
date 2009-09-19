/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/ 
#include "vtkFieldTopologyAnalyzer.h"

#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkImplicitFunction.h"
#include "vtkCutter.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkIntArray.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkDistributedFieldTracer.h"
#include "vtkPlaneSource.h"
#include "vtkMultiProcessController.h"
#include "vtkCommunicator.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkBoundingBox.h"
#include "vtkCellLocator.h"
#include "vtkTriangleFilter.h"
#include "vtkLinearSubdivisionFilter.h"

#include "vtkFieldTopologyAnalyzerUtils.h"
#include "PrintUtils.h"

#if defined vtkFieldTopologyAnalyzerDEBUG
#include <unistd.h>
#include<iostream>
using namespace std;
#endif

vtkCxxRevisionMacro(vtkFieldTopologyAnalyzer, "$Revision: 1.3 $");
vtkStandardNewMacro(vtkFieldTopologyAnalyzer);

//-----------------------------------------------------------------------------
vtkFieldTopologyAnalyzer::vtkFieldTopologyAnalyzer()
{
  #if defined vtkFieldTopologyAnalyzerDEBUG
  cerr << "====================================================================vtkFieldTopologyAnalyzer" << endl;
  #endif

  this->CutFunction=NULL;
  this->CutResolution=1;

  this->StepSize=0.001;
  this->MaxLineLength=1000;
  this->MaxNumberOfSteps=10000;

  this->SetNumberOfInputPorts(2);
  #if defined vtkFieldTopolgyAnalyzerALLOUT
  this->SetNumberOfOutputPorts(4);
  #else
  this->SetNumberOfOutputPorts(1);
  #endif

  this->Controller
    = vtkMultiProcessController::GetGlobalController();

  #if defined vtkFieldTopologyAnalyzerDEBUG
  int procId=this->Controller->GetLocalProcessId();
  int nProcs=this->Controller->GetNumberOfProcesses();
  cerr << procId <<  " " << getpid() << endl;
  #endif
}

//-----------------------------------------------------------------------------
vtkFieldTopologyAnalyzer::~vtkFieldTopologyAnalyzer()
{
  #if defined vtkFieldTopologyAnalyzerDEBUG
  cerr << "====================================================================~vtkFieldTopologyAnalyzer" << endl;
  #endif
}

//----------------------------------------------------------------------------
// Set cut function
vtkCxxSetObjectMacro(vtkFieldTopologyAnalyzer,CutFunction,vtkImplicitFunction);

//----------------------------------------------------------------------------
int vtkFieldTopologyAnalyzer::FillInputPortInformation(int port,
                                                    vtkInformation *info)
{
  #if defined vtkFieldTopologyAnalyzerDEBUG
  cerr << "====================================================================FillInputPortInformation" << endl;
  #endif
  switch (port)
    {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
      break;
    case 1:
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
int vtkFieldTopologyAnalyzer::FillOutputPortInformation(int port, vtkInformation *info)
{
  #if defined vtkFieldTopologyAnalyzerDEBUG
  cerr << "====================================================================FillOutputPortInformation" << endl;
  #endif
  // 2 Outputs:
  //   0 - Seed points
  //   1 - feild lines

  switch (port)
    {
    case 0:
    case 1:
    case 2:
    case 3:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
      break;
    default:
      vtkWarningMacro("Invalid output port requested.");
      break;
    }
  return 1;
}


//----------------------------------------------------------------------------
void vtkFieldTopologyAnalyzer::AddDatasetInputConnection(
                vtkAlgorithmOutput* algOutput)
{
  #if defined vtkFieldTopologyAnalyzerDEBUG
  cerr << "====================================================================AddDatasetInputConnectiont" << endl;
  #endif

  this->AddInputConnection(0, algOutput);
}

//----------------------------------------------------------------------------
void vtkFieldTopologyAnalyzer::ClearDatasetInputConnections()
{
  #if defined vtkFieldTopologyAnalyzerDEBUG
  cerr << "====================================================================ClearDatasetInputConnections" << endl;
  #endif

  this->SetInputConnection(0, 0);
}


//----------------------------------------------------------------------------
void vtkFieldTopologyAnalyzer::AddBoundaryInputConnection(
                vtkAlgorithmOutput* algOutput)
{
  #if defined vtkFieldTopologyAnalyzerDEBUG
  cerr << "====================================================================AddBoundaryInputConnection" << endl;
  #endif
  this->AddInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void vtkFieldTopologyAnalyzer::ClearBoundaryInputConnections()
{
  #if defined vtkFieldTopologyAnalyzerDEBUG
  cerr << "====================================================================ClearBoundaryInputConnections" << endl;
  #endif
  this->SetInputConnection(1, 0);
}

//----------------------------------------------------------------------------
int vtkFieldTopologyAnalyzer::RequestUpdateExtent(
                vtkInformation *vtkNotUsed(request),
                vtkInformationVector **inputVector,
                vtkInformationVector *outputVector)
{
  #if defined vtkFieldTopologyAnalyzerDEBUG
    cerr << "====================================================================RequestUpdateExtent" << endl;
  #endif

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int piece
    =outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces
    =outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int ghostLevel =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  // This has the effect to run the source on all procs, without it
  // only process 0 gets the source data.
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

  return 1;
}

//----------------------------------------------------------------------------
int vtkFieldTopologyAnalyzer::RequestInformation(
                vtkInformation *vtkNotUsed(request),
                vtkInformationVector **vtkNotUsed(inputVector),
                vtkInformationVector *outputVector)
{
  #if defined vtkFieldTopologyAnalyzerDEBUG
    cerr << "====================================================================RequestInformation" << endl;
  #endif

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkFieldTopologyAnalyzer::RequestData(
                vtkInformation *vtkNotUsed(request),
                vtkInformationVector **inputVector,
                vtkInformationVector *outputVector)
{
  #if defined vtkFieldTopologyAnalyzerDEBUG
  cerr << "====================================================================RequestData" << endl;
  #endif

  int procId=this->Controller->GetLocalProcessId();
  int nProcs=this->Controller->GetNumberOfProcesses();

  #if defined vtkFieldTopologyAnalyzerDEBUG
  cerr << procId <<  " " << getpid() << endl;
  #endif

  vtkInformation *inInfo;
  // Get the input on the first port. This should be a set of field lines
  // generated using the distributed field lines filter.
  inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet *inputData
    = dynamic_cast<vtkDataSet*>(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!inputData)
    {
    vtkWarningMacro("Empty input on port 0! Aborting request.");
    return 1;
    }
  // Make a copy so we can internally connect to a field tracer.
  vtkDataSet *inputCopy=inputData->NewInstance();
  inputCopy->ShallowCopy(inputData);

  // Create the seed object,
  if (!this->CutFunction)
    {
    vtkWarningMacro("Cut function has not been provided! Aborting request.");
    return 1;
    }

  vtkPolyData *seedPoints=0;
  if (this->CutFunction->IsA("vtkPlane"))
    {
    // dummy up a box that bounds the input data.
    double bounds[6];
    inputCopy->GetBounds(bounds);

    #if defined vtkFieldTopologyAnalyzerDEBUG
    cerr << procId << " Vectors local: "
         << bounds[0] << ", " << bounds[1] << ", "
         << bounds[2] << ", " << bounds[3] << ", "
         << bounds[4] << ", " << bounds[5] << endl;
    #endif

    vtkBoundingBox globalBounds;
    globalBounds.SetBounds(bounds);
    vtkCommunicator *comm=this->Controller->GetCommunicator();
    if (comm)
      {
      comm->ComputeGlobalBounds(procId, nProcs, &globalBounds);
      globalBounds.GetBounds(bounds);
      }

    #if defined vtkFieldTopologyAnalyzerDEBUG
    cerr << procId << " Vectors global: "
         << bounds[0] << ", " << bounds[1] << ", "
         << bounds[2] << ", " << bounds[3] << ", "
         << bounds[4] << ", " << bounds[5] << endl;
    #endif

    double x0[3]={bounds[0],bounds[2],bounds[4]};
    double dX[3]={bounds[1]-bounds[0],bounds[3]-bounds[2],bounds[5]-bounds[4]};
    vtkImageData *box=vtkImageData::New();
    box->SetOrigin(x0);
    box->SetSpacing(dX);
    box->SetDimensions(2,2,2);
    // cut the box.
    vtkCutter *boxCutter=vtkCutter::New();
    boxCutter->SetCutFunction(this->CutFunction);
    boxCutter->SetInput(box);
    vtkPolyData *cut=boxCutter->GetOutput();
    cut->Update();
    vtkIdType nCutPoints=cut->GetNumberOfPoints();
    if (nCutPoints<3)
      {
      vtkWarningMacro("Cut is degenerate, only " << nCutPoints << " points! Aborting request.");
      return 1;
      }
    // create the seed source. First triangulate then we can subdivide giving
    // user some control over resolution.
    vtkTriangleFilter *triangulator=vtkTriangleFilter::New();
    triangulator->SetInput(cut);
    vtkLinearSubdivisionFilter *subdividor=vtkLinearSubdivisionFilter::New();
    subdividor->SetNumberOfSubdivisions(this->CutResolution);
    subdividor->SetInput(triangulator->GetOutput());
    seedPoints=subdividor->GetOutput();
    seedPoints->Update();
    seedPoints->Register(0);
    subdividor->Delete();
    triangulator->Delete();
    boxCutter->Delete();
    box->Delete();
    }
  if (!seedPoints)
    {
    vtkWarningMacro("Failed to create seed points! Aborting request.");
    return 1;
    }

  this->UpdateProgress(0.125);

  // Trace the field lines in two steps so that we may know the end 
  // point (forward trace) and begining point (backward trace).
  // Trace field lines forward.
  vtkInformation *arrayInfo=this->GetInputArrayInformation(0);
  const char *vectors=arrayInfo->Get(vtkDataObject::FIELD_NAME());
  vtkDistributedFieldTracer *fwdFieldTracer=vtkDistributedFieldTracer::New();
  fwdFieldTracer->SetIntegrationDirectionToForward();
  fwdFieldTracer->SetIntegratorTypeToRungeKutta4();
  fwdFieldTracer->SetInitialIntegrationStep(this->StepSize);
  fwdFieldTracer->SetMinimumIntegrationStep(this->StepSize);
  fwdFieldTracer->SetMaximumIntegrationStep(100*this->StepSize);
  fwdFieldTracer->SetMaximumPropagation(this->MaxLineLength);
  fwdFieldTracer->SetMaximumNumberOfSteps(this->MaxNumberOfSteps);
  fwdFieldTracer->SetComputeVorticity(false);
  fwdFieldTracer->SetInput(inputCopy);
  fwdFieldTracer->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,vectors);
  fwdFieldTracer->SetSource(seedPoints);
  vtkPolyData *fwdFieldLines=fwdFieldTracer->GetOutput();
  fwdFieldLines->Update();
  fwdFieldLines->BuildCells();
  fwdFieldLines->Register(0);
  fwdFieldTracer->Delete();

  const size_t nFwdFieldLines=fwdFieldLines->GetNumberOfCells();
  vtkIntArray *fwdSeedPointIds
    = dynamic_cast<vtkIntArray *>(fwdFieldLines->GetPointData()->GetArray("SeedPointIds"));
  vtkDoubleArray *fwdIntegrateTime
    = dynamic_cast<vtkDoubleArray *>(fwdFieldLines->GetPointData()->GetArray("IntegrationTime"));

  this->UpdateProgress(0.35);

  // Trace field lines backward.
  vtkDistributedFieldTracer *bwdFieldTracer=vtkDistributedFieldTracer::New();
  bwdFieldTracer->SetIntegrationDirectionToBackward();
  bwdFieldTracer->SetIntegratorTypeToRungeKutta4();
  bwdFieldTracer->SetInitialIntegrationStep(this->StepSize);
  bwdFieldTracer->SetMinimumIntegrationStep(this->StepSize);
  bwdFieldTracer->SetMaximumIntegrationStep(100*this->StepSize);
  bwdFieldTracer->SetMaximumPropagation(this->MaxLineLength);
  bwdFieldTracer->SetMaximumNumberOfSteps(this->MaxNumberOfSteps);
  bwdFieldTracer->SetComputeVorticity(false);
  bwdFieldTracer->SetInput(inputCopy);
  bwdFieldTracer->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,vectors);
  bwdFieldTracer->SetSource(seedPoints);
  vtkPolyData *bwdFieldLines=bwdFieldTracer->GetOutput();
  bwdFieldLines->Update();
  bwdFieldLines->BuildCells();
  bwdFieldLines->Register(0);
  bwdFieldTracer->Delete();

  const size_t nBwdFieldLines=bwdFieldLines->GetNumberOfCells();
  vtkIntArray *bwdSeedPointIds
    = dynamic_cast<vtkIntArray *>(bwdFieldLines->GetPointData()->GetArray("SeedPointIds"));
  vtkDoubleArray *bwdIntegrateTime
    = dynamic_cast<vtkDoubleArray *>(bwdFieldLines->GetPointData()->GetArray("IntegrationTime"));

  // finished tracing field lines, no longer need the copy of the input data
  inputCopy->Delete();

  this->UpdateProgress(0.7125);


  // Make a record of the intersections here, later we compute a color. A negative
  // value is used to indicate no intersection was found.
  vtkIdType nSeedPoints=seedPoints->GetNumberOfPoints();
  vector<IntersectionSet> intersectSets(nSeedPoints);

  #if defined vtkFieldTopolgyAnalyzerALLOUT
  // the fourth output can be used to visulaize the intersection points.
  vtkPoints *intersectPts=vtkPoints::New();
  vtkCellArray *intersectCells=vtkCellArray::New();
  #endif

  // The second input should have a collection of surfaces that
  // we use to classify the streamlines. There may be more than one
  // surface input. For each we wish to find if any field line cross
  // on either forward or backward trace.
  const int nSurfaces=inputVector[1]->GetNumberOfInformationObjects();
  for (int i=0; i<nSurfaces; ++i)
    {
    // Get the surface from the input port.
    inInfo=inputVector[1]->GetInformationObject(i);
    vtkPolyData *surface
        = dynamic_cast<vtkPolyData*>(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (!surface)
      {
      vtkWarningMacro("Expected surface in polydata on input 1.");
      continue;
      }

    surface->Update();

    #if defined vtkFieldTopologyAnalyzerDEBUG
    double sbds[6];
    surface->GetBounds(sbds);
    cerr << "Surface "
         << sbds[0] << ", " << sbds[1] << ", "
         << sbds[2] << ", " << sbds[3] << ", "
         << sbds[4] << ", " << sbds[5] << endl;
    #endif

    // Initialize the locator with its cells.
    vtkCellLocator *cellLoc=vtkCellLocator::New();
    cellLoc->SetDataSet(surface);
    cellLoc->BuildLocator();

    // Test each each segment of each forward running field line
    // for intersection with the current surface.
    for (size_t j=0; j<nFwdFieldLines; ++j)
      {
      // For each segemnt of the field line piece,
      vtkIdType nCellPts;
      vtkIdType *cellPtIds;
      fwdFieldLines->GetCellPoints(j,nCellPts,cellPtIds);
      for (vtkIdType k=1; k<nCellPts; ++k)
        {
        double p0[3];
        double p1[3];
        fwdFieldLines->GetPoint(cellPtIds[k-1],p0);
        fwdFieldLines->GetPoint(cellPtIds[k],p1);
        // see if this segment intersects the surface
        double t=0.0;
        double x[3]={0.0};
        double p[3]={0.0};
        int c=0;
        int hitSurface=0;
        hitSurface=cellLoc->IntersectWithLine(p0,p1,1E-6,t,x,p,c);
        if (hitSurface)
          {
          const int seedPointId=fwdSeedPointIds->GetValue(cellPtIds[k]);
          const double integrateTime=fabs(fwdIntegrateTime->GetValue(cellPtIds[k]));
          intersectSets[seedPointId].InsertForwardIntersection(i,integrateTime);
          #if defined vtkFieldTopolgyAnalyzerALLOUT
          vtkIdType pid=intersectPts->InsertNextPoint(x);
          intersectCells->InsertNextCell(1,&pid);
          #endif
          #if defined vtkFieldTopologyAnalyzerDEBUG
          cerr << procId
               << " Forward line from seed point "
               << seedPointId
               << " intersects surface "
               << i
               << endl;
          #endif
          // stop looking after first intersect,continue to next line.
          break;
          }
        }
      }

    // Test each each segment of each backward running field line
    // for intersection with the current surface.
    for (size_t j=0; j<nBwdFieldLines; ++j)
      {
      // For each segemnt of the field line piece,
      vtkIdType nCellPts;
      vtkIdType *cellPtIds;
      bwdFieldLines->GetCellPoints(j,nCellPts,cellPtIds);
      for (vtkIdType k=1; k<nCellPts; ++k)
        {
        double p0[3];
        double p1[3];
        bwdFieldLines->GetPoint(cellPtIds[k-1],p0);
        bwdFieldLines->GetPoint(cellPtIds[k],p1);
        // see if this segment intersects the surface
        double t=0.0;
        double x[3]={0.0};
        double p[3]={0.0};
        int c=0;
        int hitSurface=0;
        hitSurface=cellLoc->IntersectWithLine(p0,p1,1E-6,t,x,p,c);
        if (hitSurface)
          {
          const int seedPointId=bwdSeedPointIds->GetValue(cellPtIds[k]);
          const double integrateTime=fabs(bwdIntegrateTime->GetValue(cellPtIds[k]));
          intersectSets[seedPointId].InsertBackwardIntersection(i,integrateTime);
          #if defined vtkFieldTopolgyAnalyzerALLOUT
          vtkIdType pid=intersectPts->InsertNextPoint(x);
          intersectCells->InsertNextCell(1,&pid);
          #endif
          #if defined vtkFieldTopologyAnalyzerDEBUG
          cerr << procId
               << " Backward line from seed point "
               << seedPointId
               << " intersects surface "
               << i
               << endl;
          #endif
          // stop looking after first intersect,continue to next line.
          break;
          }
        }
      }
    cellLoc->Delete();
    }

  this->UpdateProgress(0.92);

  // All intersections have been computed, now we just need to make a
  // reduction across all processes so each seed point is asigned
  // intersection with the surface closest along its arc. Once we have
  // the reduction we may asign a color. TODO: load ballance output here.
  vtkIntArray *intersectColor=vtkIntArray::New();
  intersectColor->SetNumberOfTuples(nSeedPoints);
  intersectColor->SetName("IntersectColor");
  seedPoints->GetPointData()->AddArray(intersectColor);
  intersectColor->Delete();
  IntersectionSetColorMapper mapper;
  mapper.SetNumberOfSurfaces(nSurfaces);
  for (int i=0; i<nSeedPoints; ++i)
    {
    IntersectionSet &set=intersectSets[i];
    set.SetSeedPointId(i);
    if (nProcs>1)
      {
      // resolve discrepency over which intersection is true. for
      // this we take the closest to the seed point across all procs
      // in each direction.
      set.AllReduce();
      }
    int color=mapper.LookupColor(set);
    intersectColor->SetValue(i,color);

    #if defined vtkFieldTopologyAnalyzerDEBUG
    cerr << endl;
    cerr << procId << " " << color << endl << set.Print() << endl;
    #endif
    }

  vtkInformation *outInfo;

  if (procId==0)
    {
    // Copy seed points into the output
    outInfo = outputVector->GetInformationObject(0);
    vtkPolyData *seedPointOutput
      = dynamic_cast<vtkPolyData*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
    seedPointOutput->ShallowCopy(seedPoints);
    }
  seedPoints->Delete();

  #if defined vtkFieldTopolgyAnalyzerALLOUT
  // Copy field lines into the output
  outInfo = outputVector->GetInformationObject(1);
  vtkPolyData *fwdFieldLineOutput =
    dynamic_cast<vtkPolyData*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  fwdFieldLineOutput->ShallowCopy(fwdFieldLines);
  // Copy field lines into the output
  outInfo = outputVector->GetInformationObject(2);
  vtkPolyData *bwdFieldLineOutput =
    dynamic_cast<vtkPolyData*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  bwdFieldLineOutput->ShallowCopy(bwdFieldLines);
  // Points of actual intersection
  outInfo = outputVector->GetInformationObject(3);
  vtkPolyData *intersectData =
    dynamic_cast<vtkPolyData*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  intersectData->SetPoints(intersectPts);
  intersectData->SetVerts(intersectCells);
  intersectPts->Delete();
  intersectCells->Delete();
  #endif

  fwdFieldLines->Delete();
  bwdFieldLines->Delete();

  return 1;
}

//----------------------------------------------------------------------------
unsigned long vtkFieldTopologyAnalyzer::GetMTime()
{
  #if defined vtkFieldTopologyAnalyzerDEBUG
  cerr << "====================================================================GetMTime" << endl;
  #endif
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if ( this->CutFunction != NULL )
    {
    time = this->CutFunction->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkFieldTopologyAnalyzer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller:    " << this->Controller << endl;
  os << indent << "CutFunction:   " << this->CutFunction << endl;
  os << indent << "CutResolution: " << this->CutResolution << endl;
  os << indent << "Cutter:        " << this->Cutter << endl;
}

