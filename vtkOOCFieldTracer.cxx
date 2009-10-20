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

#include "vtkMultiProcessController.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"
#include "vtkRungeKutta2.h"
#include "vtkRungeKutta4.h"
#include "vtkRungeKutta45.h"
#include "vtkSmartPointer.h"
#include "vtkOOCReader.h"
#include "vtkMetaDataKeys.h"

#if defined PV_3_4_BUILD
  #include "vtkInterpolatedVelocityField-3.7.h"
#else
  #include "vtkInterpolatedVelocityField.h"
#endif

#include<map>
using std::map;
using std::pair;

#include "FieldLine.h"

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
  TopologyMode(0)
{
  this->Integrator=vtkRungeKutta45::New();
  this->TermCon=new TerminationCondition;
  this->Controller=vtkMultiProcessController::GetGlobalController();
  this->SetNumberOfInputPorts(3);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkOOCFieldTracer::~vtkOOCFieldTracer()
{
  this->Integrator->Delete();
  delete this->TermCon;
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
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
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
  // 2 Outputs:
  //   0 - feild lines
  //   1 - Seed cells
  switch (port)
    {
    case 0:
    case 1:
    case 2:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
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
  //oocr->Register(0);
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
  vtkPolyData *seedSource
    = dynamic_cast<vtkPolyData*>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (seedSource==0)
    {
    vtkWarningMacro("Seed point source is not polydata. Aborting request.");
    return 1;
    }

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

  // There are two modes of operation, the traditional
  // FieldLine mode and a Topology mode. In FieldLine mode the output contains
  // field lines. In Topology mode the output contains seed cells colored by
  // where the field line terminated.
  if (this->TopologyMode)
    {
    // compute seed points (centered on cells of input). Copy the cells
    // on which we operate into the output.
    vtkIdType nLines
      =this->PolyDataToSeeds(procId,nProcs,seedSource,output,lines);
    // integrate and build surface intersection color array.
    vtkIntArray *intersectColor=vtkIntArray::New();
    intersectColor->SetNumberOfTuples(nLines);
    intersectColor->SetName("IntersectColor");
    int *pColor=intersectColor->GetPointer(0);
    double progInc=0.8/(double)nLines;
    for (vtkIdType i=0; i<nLines; ++i)
      {
      FieldLine *line=lines[i];
      this->UpdateProgress(i*progInc+0.10);
      this->OOCIntegrateOne(oocr,fieldName,line,&tcon,cache);
      pColor[i]
      =tcon.GetTerminationColor(line->GetForwardTerminator(),line->GetBackwardTerminator());
      delete line;
      lines[i]=0;
      }
    // place the color color into the polygonal cell output.
    tcon.SqueezeColorMap(intersectColor);
    output->GetCellData()->AddArray(intersectColor);
    intersectColor->Delete();
    }
  else
    {
    // compute seed points.
    vtkIdType nLines=this->PolyDataToSeeds(procId,nProcs,seedSource,lines);
    // integrate
    vtkIdType nPtsTotal=0;
    double progInc=0.8/(double)nLines;
    for (vtkIdType i=0; i<nLines; ++i)
      {
      FieldLine *line=lines[i];
      this->UpdateProgress(i*progInc+0.10);
      this->OOCIntegrateOne(oocr,fieldName,line,&tcon,cache);
      nPtsTotal+=line->GetNumberOfPoints();
      }
    // copy into output (also deletes the lines).
    this->FieldLinesToPolydata(lines,nPtsTotal,output);
    }

  oocr->Close();

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
    // static
    // vtkDataSet *nhood=0;            // data in a neighborhood of the seed point
    int numSteps=0;                 // number of steps taken in integration
    double stepSize=this->MaxStep;  // next recommended step size
    static
    vtkInterpolatedVelocityField *interp=0;

    line->GetSeedPoint(p0);
    // tcon->ResetWorkingDomain();

    // Integrate until the maximum line length is reached, maximum number of 
    // steps is reached or until a termination surface is encountered.
    while (1)
      {
      // add the point to the line.
      line->PushPoint(i,p0);

      #if defined vtkOOCFieldTracerDEBUG
      // cerr << "   " << p0[0] << ", " << p0[1] << ", " << p0[2] << endl;
      #endif

      // check integration limit
      if (lineLength>this->MaxLineLength
       || numSteps>this->MaxNumberOfSteps)
        {
        #if defined vtkOOCFieldTracerDEBUG
        cerr << "Terminated: Integration limmit exceeded." << endl;
        #endif
        line->SetTerminator(i,tcon->GetShortIntegrationId());
        break; // stop integrating
        }

      // We are now integrated through all available.
      if (tcon->OutsideProblemDomain(p0))
        {
        #if defined vtkOOCFieldTracerDEBUG
        cerr << "Terminated: Integration outside problem domain." << endl;
        #endif
        line->SetTerminator(i,tcon->GetProblemDomainSurfaceId());
        break; // stop integrating
        }

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
      interp ->FunctionValues(p0,V0);
      double speed=vtkMath::Norm(V0);
      // check for field null
      if ((speed==0) || (speed<=this->TerminalSpeed))
        {
        #if defined vtkOOCFieldTracerDEBUG
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
        #if defined vtkOOCFieldTracerDEBUG
        cerr << "Terminated: Field null encountered." << endl;
        #endif
        line->SetTerminator(i,tcon->GetFieldNullId());
        break; // stop integrating
        }

      // check for line crossing a termination surface.
      int surfIsect=tcon->SegmentTerminates(p0,p1);
      if (surfIsect)
        {
        #if defined vtkOOCFieldTracerDEBUG
        cerr << "Terminated: Surface " << surfIsect-1 << " hit." << endl;
        #endif
        line->SetTerminator(i,surfIsect);
        break;
        }

      // Update the lineLength
      lineLength+=fabs(stepTaken);

      // new start point
      p0[0]=p1[0];
      p0[1]=p1[1];
      p0[2]=p1[2];
      } /// End Integration
    // nhood->Delete();
    } /// End fwd/backwd trace
  return;
}


//-----------------------------------------------------------------------------
int vtkOOCFieldTracer::PolyDataToSeeds(
        int procId,
        int nProcs,
        vtkPolyData *seedSource,
        vtkPolyData *seedOut,
        vector<FieldLine *> &lines)
{
  // TODO to make this easier for now we only suppport polygonal cells,
  // Here we break up the work load. This assumes that the seed source is duplicated
  // on all processes. RequestInformation is configured to make this happen.
  vtkIdType nPolys=seedSource->GetNumberOfPolys();
  if (nPolys==0)
    {
    vtkWarningMacro("Did not find supported cell type in seed source. Aborting request.");
    return 0;
    }
  vtkIdType nPolysPerProc=nPolys/nProcs;
  vtkIdType nPolysLeft=nPolys%nProcs;
  vtkIdType nPolysLocal=nPolysPerProc+(procId<nPolysLeft?1:0);
  vtkIdType startPolyId=procId<nPolysLeft?procId*nPolysLocal:procId*nPolysLocal+nPolysLeft;

  // Cells are sequentially acccessed (not random) so explicitly skip all cells
  // we aren't interested in.
  vtkCellArray *seedSourcePolys=seedSource->GetPolys();
  seedSourcePolys->InitTraversal();
  for (vtkIdType i=0; i<startPolyId; ++i)
    {
    vtkIdType n;
    vtkIdType *ptIds;
    seedSourcePolys->GetNextCell(n,ptIds);
    }
  // For each cell asigned to us we'll get its center (this is the seed point)
  // and build corresponding cell in the output, The output only will have
  // the cells assigned to this process.
  vtkFloatArray *seedSourcePts
    = dynamic_cast<vtkFloatArray*>(seedSource->GetPoints()->GetData());
  if (seedSourcePts==0)
    {
    vtkWarningMacro("Seed source points are not float. Aborting request.");
    return 0;
    }
  float *pSeedSourcePts=seedSourcePts->GetPointer(0);

  // polygonal cells
  vtkFloatArray *seedOutPolyPts=vtkFloatArray::New();
  seedOutPolyPts->SetNumberOfComponents(3);
  vtkIdTypeArray *seedOutPolyCells=vtkIdTypeArray::New();
  vtkIdType nCellIds=0;
  vtkIdType nSeedOutPolyPts=0;
  vtkIdType polyId=startPolyId;

  lines.clear();
  lines.resize(nPolysLocal,0);

  map<vtkIdType,vtkIdType> idMap;

  for (vtkIdType i=0; i<nPolysLocal; ++i)
    {
    // Get the cell that belong to us.
    vtkIdType nPtIds;
    vtkIdType *ptIds;
    seedSourcePolys->GetNextCell(nPtIds,ptIds);

    // Get location to write new cell.
    vtkIdType *pSeedOutPolyCells=seedOutPolyCells->WritePointer(nCellIds,nPtIds+1);
    // update next cell write location.
    nCellIds+=nPtIds+1;
    // number of points in this cell
    *pSeedOutPolyCells=nPtIds;
    ++pSeedOutPolyCells;

    // Get location to write new point. assumes we need to copy all
    // but this is wrong as there will be many duplicates. ignored.
    float *pSeedOutPolyPts=seedOutPolyPts->WritePointer(3*nSeedOutPolyPts,3*nPtIds);
    // the seed point we will use the center of the cell
    double seed[3]={0.0};
    // transfer from input to output (only what we own)
    for (vtkIdType j=0; j<nPtIds; ++j,++pSeedOutPolyCells)
      {
      vtkIdType idx=3*ptIds[j];
      // do we already have this point?
      pair<vtkIdType,vtkIdType> elem(ptIds[j],nSeedOutPolyPts);
      pair<map<vtkIdType,vtkIdType>::iterator,bool> ret=idMap.insert(elem);
      if (ret.second==true)
        {
        // this point hasn't previsouly been coppied
        // copy the point.
        pSeedOutPolyPts[0]=pSeedSourcePts[idx  ];
        pSeedOutPolyPts[1]=pSeedSourcePts[idx+1];
        pSeedOutPolyPts[2]=pSeedSourcePts[idx+2];
        pSeedOutPolyPts+=3;

        // insert the new point id.
        *pSeedOutPolyCells=nSeedOutPolyPts;
        ++nSeedOutPolyPts;
        }
      else
        {
        // this point has been coppied, do not add a duplicate.
        // insert the other point id.
        *pSeedOutPolyCells=(*ret.first).second;
        }
      // compute contribution to cell center.
      seed[0]+=pSeedSourcePts[idx  ];
      seed[1]+=pSeedSourcePts[idx+1];
      seed[2]+=pSeedSourcePts[idx+2];
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
  seedOutPolyPts->SetNumberOfTuples(nSeedOutPolyPts);

  // put the new cells and points into the filter's output.
  vtkPoints *seedOutPolyPoints=vtkPoints::New();
  seedOutPolyPoints->SetData(seedOutPolyPts);
  seedOut->SetPoints(seedOutPolyPoints);
  seedOutPolyPoints->Delete();
  seedOutPolyPts->Delete();

  vtkCellArray *seedOutPolys=vtkCellArray::New();
  seedOutPolys->SetCells(nPolysLocal,seedOutPolyCells);
  seedOut->SetPolys(seedOutPolys);
  seedOutPolys->Delete();
  seedOutPolyCells->Delete();


  return nPolysLocal;
}

//-----------------------------------------------------------------------------
int vtkOOCFieldTracer::PolyDataToSeeds(
        int procId,
        int nProcs,
        vtkPolyData *seedSource,
        vector<FieldLine *> &lines)
{
  // TODO to make this easier for now we only suppport polygonal cells,
  // Here we break up the work load. This assumes that the seed source is duplicated
  // on all processes. RequestInformation is configured to make this happen.
  vtkIdType nPolys=seedSource->GetNumberOfPolys();
  if (nPolys==0)
    {
    vtkWarningMacro("Did not find supported cell type in seed source. Aborting request.");
    return 0;
    }
  vtkIdType nPolysPerProc=nPolys/nProcs;
  vtkIdType nPolysLeft=nPolys%nProcs;
  vtkIdType nPolysLocal=nPolysPerProc+(procId<nPolysLeft?1:0);
  vtkIdType startPolyId=procId<nPolysLeft?procId*nPolysLocal:procId*nPolysLocal+nPolysLeft;

  // Cells are sequentially acccessed (not random) so explicitly skip all cells
  // we aren't interested in.
  vtkCellArray *seedSourcePolys=seedSource->GetPolys();
  seedSourcePolys->InitTraversal();
  for (vtkIdType i=0; i<startPolyId; ++i)
    {
    vtkIdType n;
    vtkIdType *ptIds;
    seedSourcePolys->GetNextCell(n,ptIds);
    }
  // For each cell asigned to us we'll get its center (this is the seed point)
  // and build corresponding cell in the output, The output only will have
  // the cells assigned to this process.
  vtkFloatArray *seedSourcePts
    = dynamic_cast<vtkFloatArray*>(seedSource->GetPoints()->GetData());
  if (seedSourcePts==0)
    {
    vtkWarningMacro("Seed source points are not float. Aborting request.");
    return 0;
    }
  float *pSeedSourcePts=seedSourcePts->GetPointer(0);


  vtkIdType polyId=startPolyId;

  lines.resize(nPolysLocal,0);

  for (vtkIdType i=0; i<nPolysLocal; ++i)
    {
    // Get the cell that belong to us.
    vtkIdType nPtIds;
    vtkIdType *ptIds;
    seedSourcePolys->GetNextCell(nPtIds,ptIds);

    // the seed point we will use the center of the cell
    double seed[3]={0.0};
    // transfer from input to output (only what we own)
    for (vtkIdType j=0; j<nPtIds; ++j)
      {
      vtkIdType idx=3*ptIds[j];
      // compute contribution to cell center.
      seed[0]+=pSeedSourcePts[idx  ];
      seed[1]+=pSeedSourcePts[idx+1];
      seed[2]+=pSeedSourcePts[idx+2];
      }
    // finsih the seed point computation (at cell center).
    seed[0]/=nPtIds;
    seed[1]/=nPtIds;
    seed[2]/=nPtIds;

    lines[i]=new FieldLine(seed,polyId);
    ++polyId;
    }

  return nPolysLocal;
}

//-----------------------------------------------------------------------------
int vtkOOCFieldTracer::PolyDataToSeeds(
        int procId,
        int nProcs,
        vtkPointSet *seedSource,
        vector<FieldLine *> &lines)
{
  // TODO to make this easier for now we only suppport polygonal cells,
  // Here we break up the work load. This assumes that the seed source is duplicated
  // on all processes. RequestInformation is configured to make this happen.
  vtkIdType nPolys=seedSource->GetNumberOfPolys();
  if (nPolys==0)
    {
    vtkWarningMacro("Did not find supported cell type in seed source. Aborting request.");
    return 0;
    }
  vtkIdType nPolysPerProc=nPolys/nProcs;
  vtkIdType nPolysLeft=nPolys%nProcs;
  vtkIdType nPolysLocal=nPolysPerProc+(procId<nPolysLeft?1:0);
  vtkIdType startPolyId=procId<nPolysLeft?procId*nPolysLocal:procId*nPolysLocal+nPolysLeft;

  // Cells are sequentially acccessed (not random) so explicitly skip all cells
  // we aren't interested in.
  vtkCellArray *seedSourcePolys=seedSource->GetPolys();
  seedSourcePolys->InitTraversal();
  for (vtkIdType i=0; i<startPolyId; ++i)
    {
    vtkIdType n;
    vtkIdType *ptIds;
    seedSourcePolys->GetNextCell(n,ptIds);
    }
  // For each cell asigned to us we'll get its center (this is the seed point)
  // and build corresponding cell in the output, The output only will have
  // the cells assigned to this process.
  vtkFloatArray *seedSourcePts
    = dynamic_cast<vtkFloatArray*>(seedSource->GetPoints()->GetData());
  if (seedSourcePts==0)
    {
    vtkWarningMacro("Seed source points are not float. Aborting request.");
    return 0;
    }
  float *pSeedSourcePts=seedSourcePts->GetPointer(0);


  vtkIdType polyId=startPolyId;

  lines.resize(nPolysLocal,0);

  for (vtkIdType i=0; i<nPolysLocal; ++i)
    {
    // Get the cell that belong to us.
    vtkIdType nPtIds;
    vtkIdType *ptIds;
    seedSourcePolys->GetNextCell(nPtIds,ptIds);

    // the seed point we will use the center of the cell
    double seed[3]={0.0};
    // transfer from input to output (only what we own)
    for (vtkIdType j=0; j<nPtIds; ++j)
      {
      vtkIdType idx=3*ptIds[j];
      // compute contribution to cell center.
      seed[0]+=pSeedSourcePts[idx  ];
      seed[1]+=pSeedSourcePts[idx+1];
      seed[2]+=pSeedSourcePts[idx+2];
      }
    // finsih the seed point computation (at cell center).
    seed[0]/=nPtIds;
    seed[1]/=nPtIds;
    seed[2]/=nPtIds;

    lines[i]=new FieldLine(seed,polyId);
    ++polyId;
    }

  return nPolysLocal;
}


//-----------------------------------------------------------------------------
int vtkOOCFieldTracer::FieldLinesToPolydata(
        vector<FieldLine *> &lines,
        vtkIdType nPtsTotal,
        vtkPolyData *fieldLines)
{
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
  #if defined vtkOOCFieldTracerDEBUG
  // cell data
  vtkFloatArray *lineIds=vtkFloatArray::New();
  lineIds->SetName("line id");
  lineIds->SetNumberOfTuples(nLines);
  float *pLineIds=lineIds->GetPointer(0);
  #endif
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
    #if defined vtkOOCFieldTracerDEBUG
    // build the cell data
    *pLineIds=lines[i]->GetSeedId();
    ++pLineIds;
    #endif

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
  #if defined vtkOCCFieldTracerDEBUG
  // cell data
  fieldLines->GetCellData()->AddArray(lineIds);
  lineIds->Delete();
  #endif

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
