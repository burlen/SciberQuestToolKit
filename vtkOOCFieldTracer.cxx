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

vtkCxxRevisionMacro(vtkOOCFieldTracer, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkOOCFieldTracer);

const double vtkOOCFieldTracer::EPSILON = 1.0E-12;

//=============================================================================
class FieldLine
{
public:
  FieldLine(double p[3], int seedId=0)
      :
    FwdTrace(0),
    BwdTrace(0),
    SeedId(seedId),
    FwdTerminator(0),
    BwdTerminator(0)
    {
    this->Seed[0]=p[0];
    this->Seed[1]=p[1];
    this->Seed[2]=p[2];

    this->FwdTrace=vtkFloatArray::New();
    this->FwdTrace->SetNumberOfComponents(3);
    this->FwdTrace->Allocate(128);
    this->BwdTrace=vtkFloatArray::New();
    this->BwdTrace->SetNumberOfComponents(3);
    this->BwdTrace->Allocate(128);
    }
  FieldLine(const FieldLine &other)
    {
    *this=other;
    }
  ~FieldLine()
    {
    this->FwdTrace->Delete();
    this->BwdTrace->Delete();
    }
  const FieldLine &operator=(const FieldLine &other)
    {
    if (&other==this)
      {
      return *this;
      }
    this->Seed[0]=other.Seed[0];
    this->Seed[1]=other.Seed[1];
    this->Seed[2]=other.Seed[2];

    this->SeedId=other.SeedId;

    this->FwdTerminator=other.FwdTerminator;
    this->BwdTerminator=other.BwdTerminator;

    this->FwdTrace->Delete();
    this->FwdTrace=other.FwdTrace;
    this->FwdTrace->Register(0);
    this->BwdTrace->Delete();
    this->BwdTrace=other.BwdTrace;
    this->BwdTrace->Register(0);
    }
  void PushPoint(int dir,float *p)
    {
    assert((dir>=0)&&(dir<=1));
    vtkFloatArray *line=dir==0?BwdTrace:FwdTrace;
    line->InsertNextTuple(p);
    }
  void PushPoint(int dir,double *p)
    {
    assert((dir>=0)&&(dir<=1));
    vtkFloatArray *line=dir==0?BwdTrace:FwdTrace;
    line->InsertNextTuple(p);
    }
  void SetTerminator(int dir, int code)
    {
    assert((dir>=0)&&(dir<=1));
    int *term=dir==0?&this->BwdTerminator:&this->FwdTerminator;
    *term=code;
    }

  int GetForwardTerminator() const
    {
    return this->FwdTerminator;
    }
  int GetBackwardTerminator() const
    {
    return this->BwdTerminator;
    }
  int GetSeedId() const
    {
    return this->SeedId;
    }
  float *GetSeedPoint()
    {
    return this->Seed;
    }
  const float *GetSeedPoint() const
    {
    return this->Seed;
    }
  void GetSeedPoint(float p[3]) const
    {
    p[0]=this->Seed[0];
    p[1]=this->Seed[1];
    p[2]=this->Seed[2];
    }
  void GetSeedPoint(double p[3]) const
    {
    p[0]=this->Seed[0];
    p[1]=this->Seed[1];
    p[2]=this->Seed[2];
    }
  vtkIdType GetNumberOfPoints()
    {
    return
    this->FwdTrace->GetNumberOfTuples()+this->BwdTrace->GetNumberOfTuples()-1;
    // less one because seed point is duplicated in both.
    }
  vtkIdType CopyPointsTo(float *pts)
    {
    // Copy the bwd running field line, reversing its order
    // so it ends on the seed point.
    vtkIdType nPtsBwd=this->BwdTrace->GetNumberOfTuples();
    float *pbtr=this->BwdTrace->GetPointer(0);
    pbtr+=3*nPtsBwd-3;
    for (vtkIdType i=0; i<nPtsBwd; ++i,pts+=3,pbtr-=3)
      {
      pts[0]=pbtr[0];
      pts[1]=pbtr[1];
      pts[2]=pbtr[2];
      }
    // Copy the forward running field line, skip the
    // seed point as we already coppied it.
    vtkIdType nPtsFwd=this->FwdTrace->GetNumberOfTuples();
    float *pftr=this->FwdTrace->GetPointer(0);
    pftr+=3; // skip
    for (vtkIdType i=1; i<nPtsFwd; ++i,pts+=3,pftr+=3)
      {
      pts[0]=pftr[0];
      pts[1]=pftr[1];
      pts[2]=pftr[2];
      }
    return nPtsBwd+nPtsFwd-1;
    }

private:
  FieldLine();

private:
  vtkFloatArray *FwdTrace;
  vtkFloatArray *BwdTrace;
  float Seed[3];
  int SeedId;
  int FwdTerminator;
  int BwdTerminator;
};


//*****************************************************************************


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
  OOCNeighborhoodSize(15)
{
  this->Integrator=vtkRungeKutta45::New();
  this->TermCon=new TerminationCondition;
  this->Controller=vtkMultiProcessController::GetGlobalController();
  this->SetNumberOfInputPorts(3);
  #if defined vtkOOCFieldTracerAllOut
  this->SetNumberOfOutputPorts(3);
  #else
  this->SetNumberOfOutputPorts(2);
  #endif
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
  // 2 Outputs:
  //   0 - feild lines
  //   1 - Seed points
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
  int nSources=inputVector[0]->GetNumberOfInformationObjects();
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
  nSources=inputVector[1]->GetNumberOfInformationObjects();
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

  #if defined vtkOOCFieldTracerDEBUG
  cerr << procId <<  " " << getpid() << endl;
  #endif

  vtkInformation *info;
  // Get the input on the first port. This should be the dummy dataset 
  // produced by ameta-reader. The information object should have the
  // OOC Reader.
  info = inputVector[0]->GetInformationObject(0);
  if (!info->Has(vtkOOCReader::READER()))
    {
    vtkWarningMacro(
        "OOCReader object not present in input pipeline "
        "information! Aborting request.");
    return 1;
    }
  vtkOOCReader *oocr
    = dynamic_cast<vtkOOCReader*>(info->Get(vtkOOCReader::READER()));
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

  // Get seed points from the required second input, at the same time
  // we'll construct the output.
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

  // TODO to make this easier for now we only suppport polygonal cells,
  // Here we break up the work load. This assumes that the seed source is duplicated
  // on all processes. RequestInformation is configured to make this happen.
  vtkIdType nPolys=seedSource->GetNumberOfPolys();
  if (nPolys==0)
    {
    vtkWarningMacro("Did not find supported cell type in seed source. Aborting request.");
    return 1;
    }
  vtkIdType nPolysPerProc=nPolys/nProcs;
  vtkIdType nPolysLeft=nPolys%nProcs;
  vtkIdType nPolysLocal=nPolysPerProc+(procId<nPolysLeft?1:0);
  vtkIdType startPolyId=procId<nPolysLeft?procId*nPolysLocal:procId*nPolysLocal+nPolysLeft;
  // vtkIdType endPolyId=startPolyId+nPolysLocal;

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
    return 1;
    }
  float *pSeedSourcePts=seedSourcePts->GetPointer(0);

  // polygonal cells
  vtkFloatArray *seedOutPolyPts=vtkFloatArray::New();
  seedOutPolyPts->SetNumberOfComponents(3);
  vtkIdTypeArray *seedOutPolyCells=vtkIdTypeArray::New();
  vtkIdType nCellIds=0;
  vtkIdType nSeedOutPolyPts=0;
  vtkIdType polyId=startPolyId;

  #if defined vtkOOCFieldTracerAllOut
  // polygonal cell centers
  vtkFloatArray *seedOutCellCenterPts=vtkFloatArray::New();
  seedOutCellCenterPts->SetNumberOfComponents(3);
  seedOutCellCenterPts->SetNumberOfTuples(nPolysLocal);
  float *pSeedOutCellCenterPts=seedOutCellCenterPts->GetPointer(0);
  vtkIdTypeArray *seedOutCellCenterCells=vtkIdTypeArray::New();
  seedOutCellCenterCells->SetNumberOfTuples(2*nPolysLocal);
  vtkIdType *pSeedOutCellCenterCells=seedOutCellCenterCells->GetPointer(0);
  #endif

  vector<FieldLine *> lines(nPolysLocal,0);

  map<vtkIdType,vtkIdType> idMap;

  for (vtkIdType i=0; i<nPolysLocal; ++i)
    {
    // Get the cell that belong to us.
    vtkIdType nPtIds;
    vtkIdType *ptIds;
    seedSourcePolys->GetNextCell(nPtIds,ptIds);

    /// cerr << ptIds[0];
    /// for (int q=1; q<nPtIds; ++q){cerr << ", " << ptIds[q];}
    /// cerr << endl;

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
        /// cerr << "    " << ptIds[j] << " -> " << nSeedOutPolyPts << endl;
        // this point hasn't previsouly been coppied
        // copy the point.
        pSeedOutPolyPts[0]=pSeedSourcePts[idx  ];
        pSeedOutPolyPts[1]=pSeedSourcePts[idx+1];
        pSeedOutPolyPts[2]=pSeedSourcePts[idx+2];

        /// cerr << pSeedOutPolyPts[0] << ", " << pSeedOutPolyPts[1] << ", " << pSeedOutPolyPts[2] << endl;

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

    #if defined vtkOOCFieldTracerAllOut
    pSeedOutCellCenterPts[0]=seed[0];
    pSeedOutCellCenterPts[1]=seed[1];
    pSeedOutCellCenterPts[2]=seed[2];
    pSeedOutCellCenterPts+=3;

    pSeedOutCellCenterCells[0]=1;
    pSeedOutCellCenterCells[1]=i;
    pSeedOutCellCenterCells+=2;
    #endif

    lines[i]=new FieldLine(seed,polyId);
    ++polyId;
    }

  // correct the length of the point array, above we assumed 
  // that all points from each cell needed to be inserted
  // and allocated that much space.
  seedOutPolyPts->SetNumberOfTuples(nSeedOutPolyPts);

  // put the new cells and points into the filter's output.
  info=outputVector->GetInformationObject(1);
  vtkPolyData *seedOutPolyPd
    = dynamic_cast<vtkPolyData*>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (seedOutPolyPd==0)
    {
    vtkWarningMacro("Seed point output is not polydata. Aborting request.");
    return 1;
    }
  vtkPoints *seedOutPolyPoints=vtkPoints::New();
  seedOutPolyPoints->SetData(seedOutPolyPts);
  seedOutPolyPd->SetPoints(seedOutPolyPoints);
  seedOutPolyPoints->Delete();
  seedOutPolyPts->Delete();

  vtkCellArray *seedOutPolys=vtkCellArray::New();
  seedOutPolys->SetCells(nPolysLocal,seedOutPolyCells);
  seedOutPolyPd->SetPolys(seedOutPolys);
  seedOutPolys->Delete();
  seedOutPolyCells->Delete();

  #if defined vtkOOCFieldTracerAllOut
  // put the new verts into the filters output.
  info=outputVector->GetInformationObject(2);
  vtkPolyData *seedOutCellCenterPd
    = dynamic_cast<vtkPolyData*>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (seedOutCellCenterPd==0)
    {
    vtkWarningMacro("Seed point output is not polydata. Aborting request.");
    return 1;
    }
  // points
  vtkPoints *seedOutCellCenterPoints=vtkPoints::New();
  seedOutCellCenterPoints->SetData(seedOutCellCenterPts);
  seedOutCellCenterPd->SetPoints(seedOutCellCenterPoints);
  seedOutCellCenterPoints->Delete();
  seedOutCellCenterPts->Delete();
  // cells
  vtkCellArray *seedOutCellCenterVerts=vtkCellArray::New();
  seedOutCellCenterVerts->SetCells(nPolysLocal,seedOutCellCenterCells);
  seedOutCellCenterPd->SetVerts(seedOutCellCenterVerts);
  seedOutCellCenterVerts->Delete();
  seedOutCellCenterCells->Delete();
  #endif


  /*
  cerr << endl << endl;
  int nTups=seedOutPts->GetNumberOfTuples();
  for (int i=0; i<nTups; ++i)
    {
    double t[3];
    seedOutPts->GetTuple(i,t);
    cerr << t[0] << ", " << t[1] << ", " << t[2] << endl;
    }*/

  // put the points and cells just defined into the output.
  /// cerr << "npts " << seedOutPts->GetNumberOfTuples() << endl;
  /// vtkIdType *pc=seedOutCells->GetPointer(0);
  /// int nTup=seedOutCells->GetNumberOfTuples();
  /// for (int i=0; i<nTup;++i)
  ///   {
  ///   cerr << pc[i] << ", ";
  ///   }

  /// seedOutPolyPd->Print(cerr);
  /// seedOutCellCenterPd->Print(cerr);


  // integrate and build surface intersection color array.
  vtkIntArray *intersectColor=vtkIntArray::New();
  intersectColor->SetNumberOfTuples(nPolysLocal);
  intersectColor->SetName("IntersectColor");
  int *pColor=intersectColor->GetPointer(0);
  vtkIdType nLinePtsAll=0;
  double progInc=0.8/(double)nPolysLocal;
  for (vtkIdType i=0; i<nPolysLocal; ++i)
    {
    FieldLine *line=lines[i];
    this->UpdateProgress(i*progInc+0.10);
    this->OOCIntegrateOne(oocr,fieldName,line,&tcon);
    pColor[i]
    =tcon.GetTerminationColor(line->GetForwardTerminator(),line->GetBackwardTerminator());
    nLinePtsAll+=line->GetNumberOfPoints();
    }
  // place the color color into the polygonal cell output.
  tcon.SqueezeColorMap(intersectColor);
  seedOutPolyPd->GetCellData()->AddArray(intersectColor);
  intersectColor->Delete();

  // construct lines from the integrations.
  vtkFloatArray *linePts=vtkFloatArray::New();
  linePts->SetNumberOfComponents(3);
  linePts->SetNumberOfTuples(nLinePtsAll);
  float *pLinePts=linePts->GetPointer(0);
  vtkIdTypeArray *lineCells=vtkIdTypeArray::New();
  lineCells->SetNumberOfTuples(nLinePtsAll+nPolysLocal);
  vtkIdType *pLineCells=lineCells->GetPointer(0);
  vtkIdType ptId=0;
  for (vtkIdType i=0; i<nPolysLocal; ++i)
    {
    // copy the points
    vtkIdType nLinePts=lines[i]->CopyPointsTo(pLinePts);
    pLinePts+=3*nLinePts;
    delete lines[i];

    // build the cell
    *pLineCells=nLinePts;
    ++pLineCells;
    for (vtkIdType q=0; q<nLinePts; ++q)
      {
      *pLineCells=ptId;
      ++pLineCells;
      ++ptId;
      }
    }

  // //
  // cerr << "nLinePtsAll=" << nLinePtsAll << endl;
  // cerr << endl;
  // cerr << "LinePts=(" << endl;
  // float *plp=linePts->GetPointer(0);
  // for (vtkIdType i=0; i<nLinePtsAll; ++i)
  //   {
  //   cerr << i << " -> " << plp[0] << "," << plp[1] << ", " << plp[2] << endl;
  //   plp+=3;
  //   }
  // cerr << ")" << endl;
  // //
  // cerr << "LineCells=(" << endl;
  // vtkIdType *pc=lineCells->GetPointer(0);
  // for (vtkIdType i=0; i<nLinePtsAll+nPolysLocal; ++i)
  //   {
  //   cerr << " " << pc[i];
  //   }
  // cerr << ")" << endl;

  // put the lines into the output.
  info=outputVector->GetInformationObject(0);
  vtkPolyData *fieldLines
    = dynamic_cast<vtkPolyData*>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (fieldLines==0)
    {
    vtkWarningMacro("Failed to get field line output. Aborting request.");
    return 1;
    }
  // points
  vtkPoints *fieldLinePoints=vtkPoints::New();
  fieldLinePoints->SetData(linePts);
  fieldLines->SetPoints(fieldLinePoints);
  fieldLinePoints->Delete();
  linePts->Delete();
  // cells
  vtkCellArray *fieldLineCells=vtkCellArray::New();
  fieldLineCells->SetCells(nPolysLocal,lineCells);
  fieldLines->SetLines(fieldLineCells);
  fieldLineCells->Delete();
  lineCells->Delete();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkOOCFieldTracer::OOCIntegrateOne(
      vtkOOCReader *oocR,
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
    vtkDataSet *nhood=vtkPolyData::New();// data in a neighborhood of the seed point
    int numSteps=0;                 // number of steps taken in integration
    double stepSize=this->MaxStep;  // next recommended step size
    vtkInterpolatedVelocityField *interp;

    line->GetSeedPoint(p0);

    // Integrate until the maximum line length is reached, maximum number of 
    // steps is reached or until a termination surface is encountered.
    while (1)
      {
      // add the point to the line.
      line->PushPoint(i,p0);

      #if defined vtkOOCFieldTracerDEBUG
      cerr << "   " << p0[0] << ", " << p0[1] << ", " << p0[2] << endl;
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
        nhood->Delete();
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
    nhood->Delete();
    } /// End fwd/backwd trace
  return;
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
