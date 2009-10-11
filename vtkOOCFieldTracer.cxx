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

#if defined PV_3_4_BUILD
  #include "vtkInterpolatedVelocityField-3.7.h"
#else
  #include "vtkInterpolatedVelocityField.h"
#endif

#include<map>
using std::map;
using std::pair;

vtkCxxRevisionMacro(vtkOOCFieldTracer, "$Revision: 1.49 $");
vtkStandardNewMacro(vtkOOCFieldTracer);

const double vtkOOCFieldTracer::EPSILON = 1.0E-12;

//=============================================================================
class _Interval
{
public:
  _Interval()
      :
    Interval(0.0),
    Unit(LENGTH_UNIT)
      {}
  double Interval;
  int Unit;
  enum Units
  {
    LENGTH_UNIT = 1,
    CELL_LENGTH_UNIT = 2
  };
};

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
    vtkFloatArray *line=dir==0?FwdTrace:BwdTrace;
    line->InsertNextTuple(p);
    }
  void PushPoint(int dir,double *p)
    {
    assert((dir>=0)&&(dir<=1));
    vtkFloatArray *line=dir==0?FwdTrace:BwdTrace;
    line->InsertNextTuple(p);
    }
  void SetTerminator(int dir, int code)
    {
    assert((dir>=0)&&(dir<=1));
    int *term=dir==0?&this->FwdTerminator:&this->BwdTerminator;
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
  StepUnit(_Interval::CELL_LENGTH_UNIT),
  InitialStep(0.1),
  MinStep(0.1),
  MaxStep(0.5),
  MaxError(1E-5),
  MaxNumberOfSteps(1000),
  MaxLineLength(1E6),
  TerminalSpeed(1E-6)
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
  if (unit!=_Interval::LENGTH_UNIT
   && unit!=_Interval::CELL_LENGTH_UNIT )
    {
    unit=_Interval::CELL_LENGTH_UNIT;
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
    tcon.PushSurface(pd);
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
    pLinePts+=nLinePts;
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

//  vtkPoints *seedPts=seeds->GetPoints();
//   vtkIdType nSeedPts=seedPts->GetNumberOfPoints();
//   vtkIdType seedPtsPerProc=nSeedPts/nProcs;
//   vtkIdType seedPtsLeft=nSeedPts%nProcs;
//   vtkIdType seedPtsLocal=seedPtsPerProc+(procId<seedPtsLeft?1:0);
//   vtkIdType startPtId=procId<seedPtsLeft?procId*seedPtsLocal:procId*seedPtsLocal+seedPtsLeft;
//   vtkIdType endPtId=startPtId+seedPtsLocal;
//   double progInc=0.9/(double)seedPtsLocal;
//   lines.resize(nSeedPtsLocal,0);
//   for (vtkIdType j=startPtId, q=0; j<endPtId; ++j, ++q)
//     {
//     this->UdpateProgress(q*progInc);
//     lines[q]=new FieldLine(seedPts->GetPoint(j),j);
//     this->OOCIntegrateOne(oocr,lines[q],tcon);
//     }


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
    vtkDataSet *nhood=vtkPolyData::New();// data in a neighborhood about the seed point
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

      cerr << "   " << p0[0] << ", " << p0[1] << ", " << p0[2] << endl;

      // check integration limit
      if (lineLength>this->MaxLineLength
       || numSteps>this->MaxNumberOfSteps)
        {
        cerr << "Terminated: Integration limmit exceeded." << endl;
        line->SetTerminator(i,tcon->GetShortIntegrationId());
        break; // stop integrating
        }

      // We are now integrated through all available.
      if (tcon->OutsideProblemDomain(p0))
        {
        cerr << "Terminated: Integration outside problem domain." << endl;
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
        cerr << "Terminated: Field null encountered." << endl;
        line->SetTerminator(i,tcon->GetFieldNullId());
        break; // stop integrating
        }

      // Convert intervals from cell fractions into lengths.
      vtkCell* cell=nhood->GetCell(interp->GetLastCellId());
      double cellLength=sqrt(cell->GetLength2());
      this->ClipStep(stepSize,stepSign,minStep,maxStep,cellLength,lineLength);

      /**
      // TODO there is quite a lot of logic related to integrate direction about
      // would is make more sense to always assume a forward direction and flip sign
      // on the vector field itself?

      // If, with the next step, lineLength will be larger than
      // max, reduce it so that it is (approximately) equal to max.
      _Interval aStep;
      aStep.Interval=fabs(stepSize);
      if ((lineLength+aStep.Interval)>this->MaxLineLength)
        {
        aStep.Interval=this->MaxLineLength-lineLength;
        stepSize=stepSize>=0?1.0:-1.0;
        stepSize*=this->ConvertToLength(aStep.Interval,aStep.Unit,cellLength);
        maxStep=stepSize;
        }*/

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

      /// cerr << stepSize << "==" << stepTaken << endl;

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
        cerr << "Terminated: Field null encountered." << endl;
        line->SetTerminator(i,tcon->GetFieldNullId());
        break; // stop integrating
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
  if (unit==_Interval::LENGTH_UNIT )
    {
    retVal=interval;
    }
  else
  if (unit==_Interval::CELL_LENGTH_UNIT)
    {
    retVal=interval*cellLength;
    }
  return retVal;
}

//-----------------------------------------------------------------------------
void vtkOOCFieldTracer::ConvertIntervals(
      double& step,
      double& minStep,
      double& maxStep,
      int direction,
      double cellLength)
{
  minStep = maxStep = step = 
    direction * this->ConvertToLength( this->InitialStep,
                                       this->StepUnit, cellLength );

  if ( this->MinStep > 0.0 )
    {
    minStep = this->ConvertToLength( this->MinStep,
                                     this->StepUnit, cellLength );
    }

  if ( this->MaxStep > 0.0 )
    {
    maxStep = this->ConvertToLength( this->MaxStep,
                                     this->StepUnit, cellLength );
    }
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

















// vtkExecutive* vtkOOCFieldTracer::CreateDefaultExecutive()
// {
//   return vtkCompositeDataPipeline::New();
// }

// //-----------------------------------------------------------------------------
// void vtkOOCFieldTracer::Integrate(
//       vtkDataSet *input0,
//       vtkPolyData* output,
//       vtkDataArray* seedSource, 
//       vtkIdList* seedIds,
//       vtkIntArray* integrationDirections,
//       double lastPoint[3],
//       vtkInterpolatedVelocityField* func,
//       int maxCellSize,
//       const char *vecName,
//       double& inPropagation,
//       vtkIdType& inNumSteps)
// {
//   int i;
//   vtkIdType numLines = seedIds->GetNumberOfIds();
//   double propagation = inPropagation;
//   vtkIdType numSteps = inNumSteps;
// 
//   // Useful pointers
//   vtkDataSetAttributes* outputPD = output->GetPointData();
//   vtkDataSetAttributes* outputCD = output->GetCellData();
// 
// 
//   int direction=1;
// 
//   double* weights = 0;
//   if ( maxCellSize > 0 )
//     {
//     weights = new double[maxCellSize];
//     }
// 
//   if (this->GetIntegrator() == 0)
//     {
//     vtkErrorMacro("No integrator is specified.");
//     return;
//     }
// 
//   // Used in GetCell() 
//   vtkGenericCell* cell = vtkGenericCell::New();
// 
//   // Create a new integrator, the type is the same as Integrator
//   vtkInitialValueProblemSolver* integrator=this->GetIntegrator()->NewInstance();
//   integrator->SetFunctionSet(func);
// 
//   // Since we do not know what the total number of points
//   // will be, we do not allocate any. This is important for
//   // cases where a lot of streamers are used at once. If we
//   // were to allocate any points here, potentially, we can
//   // waste a lot of memory if a lot of streamers are used.
//   // Always insert the first point
//   vtkPoints* outputPoints = vtkPoints::New();
//   vtkCellArray* outputLines = vtkCellArray::New();
// 
//   // We will keep track of integration time in this array
//   vtkDoubleArray* time = vtkDoubleArray::New();
//   time->SetName("IntegrationTime");
// 
//   // This array explains why the integration stopped
//   vtkIntArray* retVals = vtkIntArray::New();
//   retVals->SetName("ReasonForTermination");
// 
//   // This array assigns a unique id to each line. This does 
//   // 2 things for you, 1) allows for treatment of line as a unit
//   // and 2) lets you do a reverse lookup given the line get the
//   // seed point.
//   vtkIntArray *seedPointIds=vtkIntArray::New();
//   seedPointIds->SetName("SeedPointIds");
// 
//   vtkDoubleArray* cellVectors = 0;
//   vtkDoubleArray* vorticity = 0;
//   vtkDoubleArray* rotation = 0;
//   vtkDoubleArray* angularVel = 0;
//   if (this->ComputeVorticity)
//     {
//     cellVectors = vtkDoubleArray::New();
//     cellVectors->SetNumberOfComponents(3);
//     cellVectors->Allocate(3*VTK_CELL_SIZE);
// 
//     vorticity = vtkDoubleArray::New();
//     vorticity->SetName("Vorticity");
//     vorticity->SetNumberOfComponents(3);
// 
//     rotation = vtkDoubleArray::New();
//     rotation->SetName("Rotation");
// 
//     angularVel = vtkDoubleArray::New();
//     angularVel->SetName("AngularVelocity");
//     }
//   // We will interpolate all point attributes of the input on
//   // each point of the output (unless they are turned off)
//   // Note that we are using only the first input, if there are more
//   // than one, the attributes have to match.
//   outputPD->InterpolateAllocate(input0->GetPointData());
//   // Note:  It is an overestimation to have the estimate the same number of
//   // output points and input points.  We sill have to squeeze at end.
//   // TODO that seems like a gross overestimation....
// 
//   vtkIdType numPtsTotal=0;
//   double velocity[3];
// 
//   int shouldAbort = 0;
// 
//   // Work each line by line, note a line is not a seed because each seed
//   // may be traced in both directions producing two lines.
//   /// TODO here is where work queue should be accessed.
//   for(int currentLine = 0; currentLine < numLines; currentLine++)
//     {
// 
//     double progress = static_cast<double>(currentLine)/numLines;
//     this->UpdateProgress(progress);
// 
//     switch (integrationDirections->GetValue(currentLine))
//       {
//       case FORWARD:
//         direction = 1;
//         break;
//       case BACKWARD:
//         direction = -1;
//         break;
//       }
// 
//     // temporary variables used in the integration
//     double point1[3], point2[3], pcoords[3], vort[3], omega;
//     vtkIdType index, numPts=0;
// 
//     // Clear the last cell to avoid starting a search from
//     // the last point in the streamline
//     func->ClearLastCellId();
// 
//     // Initial point, if interpolation fails then skip this
//     // line.
//     seedSource->GetTuple(seedIds->GetId(currentLine), point1);
//     memcpy(point2, point1, 3*sizeof(double));
//     if (!func->FunctionValues(point1, velocity))
//       {
//       continue; // next line
//       }
// 
//     // Stop integrating a line at predetermined length and step
//     // limits.
//     if(propagation>=this->MaximumPropagation
//       || numSteps>this->MaximumNumberOfSteps)
//       {
//       continue; // next line
//       }
// 
//     // Insert the starting point into the output.
//     numPts++;
//     numPtsTotal++;
//     vtkIdType nextPoint = outputPoints->InsertNextPoint(point1);
//     time->InsertNextValue(0.0);
// 
//     // We will always pass an arc-length step size to the integrator.
//     // If the user specifies a step size in cell length unit, we will 
//     // have to convert it to arc length.
//     Interval stepSize;  // either positive or negative
//     stepSize.Unit  = LENGTH_UNIT;
//     stepSize.Interval = 0;
//     Interval aStep; // always positive
//     aStep.Unit = LENGTH_UNIT;
//     double step, minStep=0, maxStep=0;
//     double stepTaken, accumTime=0;
//     double speed;
//     double cellLength;
//     int retVal=OUT_OF_LENGTH;
// 
//     // Get the input data. Make sure we use the same dataset as the
//     // interpolator.
//     vtkDataSet *input=func->GetLastDataSet();
//     vtkPointData *inputPD=input->GetPointData();
//     vtkDataArray *inVectorsinVectors=inputPD->GetVectors(vecName);
// 
//     // Convert intervals to arc-length unit
//     input->GetCell(func->GetLastCellId(), cell);
//     cellLength = sqrt(static_cast<double>(cell->GetLength2()));
//     speed = vtkMath::Norm(velocity);
//     // Never call conversion methods if speed == 0
//     if (speed!=0.0)
//       {
//       this->ConvertIntervals(stepSize.Interval,minStep,maxStep,direction,cellLength);
//       }
// 
//     // Interpolate all point attributes on first point
//     func->GetLastWeights(weights);
//     outputPD->InterpolatePoint(inputPD,nextPoint,cell->PointIds,weights);
// 
//     // Compute vorticity if required
//     // This can be used later for streamribbon generation.
//     if (this->ComputeVorticity)
//       {
//       inVectors->GetTuples(cell->PointIds, cellVectors);
//       func->GetLastLocalCoordinates(pcoords);
//       vtkOOCFieldTracer::CalculateVorticity(cell, pcoords, cellVectors, vort);
//       vorticity->InsertNextTuple(vort);
//       // rotation
//       // local rotation = vorticity . unit tangent ( i.e. velocity/speed )
//       if (speed != 0.0)
//         {
//         omega = vtkMath::Dot(vort,velocity);
//         omega /= speed;
//         omega *= this->RotationScale;
//         }
//       else
//         {
//         omega = 0.0;
//         }
//       angularVel->InsertNextValue(omega);
//       rotation->InsertNextValue(0.0);
//       }
// 
//     // Add seed point id. This can be used to select line
//     // as a unit, and for reverse lookup, given the line
//     // look up the seed point.
//     vtkIdType seedPointId=seedIds->GetId(currentLine);
//     seedPointIds->InsertNextValue(seedPointId);
// 
// 
//     double error = 0;
//     /// line integration.
//     // Integrate until the maximum propagation length is reached, 
//     // maximum number of steps is reached or until a boundary is encountered.
//     // Begin Integration
//     while (propagation<this->MaximumPropagation)
//       {
//       // check for step limit
//       if (numSteps>this->MaximumNumberOfSteps)
//         {
//         retVal = OUT_OF_STEPS;
//         break; // stop integrating
//         }
//       // check for field null
//       if ( (speed == 0) || (speed <= this->TerminalSpeed) )
//         {
//         retVal = STAGNATION;
//         break; // stop integrating 
//         }
// 
//       // // every 1000 steps report progress...how about every field line?
//       // if ( numSteps++ % 1000 == 1 )
//       //   {
//       //   progress = 
//       //     ( currentLine + propagation / this->MaximumPropagation ) / numLines;
//       //   this->UpdateProgress(progress);
//       // 
//       //   if (this->GetAbortExecute())
//       //     {
//       //     shouldAbort = 1;
//       //     break;
//       //     }
//       //   }
// 
//       // If, with the next step, propagation will be larger than
//       // max, reduce it so that it is (approximately) equal to max.
//       aStep.Interval=fabs(stepSize.Interval);
//       if ((propagation+aStep.Interval)>this->MaximumPropagation)
//         {
//         aStep.Interval=this->MaximumPropagation-propagation;
//         // Keep the sign of the step size intacted
//         if (stepSize.Interval>=0.0)
//           {
//           stepSize.Interval=this->ConvertToLength(aStep,cellLength);
//           }
//         else
//           {
//           stepSize.Interval=this->ConvertToLength(aStep,cellLength)*(-1.0);
//           }
//         maxStep=stepSize.Interval;
//         }
//       this->LastUsedStepSize = stepSize.Interval;
// 
//       /// Integrate
//       // Calculate the next step using the integrator provided.
//       // Note, both stepSizeInterval and stepTaken are modified
//       // by the rk45 integrator.
//       func->SetNormalizeVector(true);
//       int out=integrator->ComputeNextStep(point1,point2,0,stepSize.Interval,
//                                       stepTaken,minStep,maxStep,
//                                       this->MaximumError,error);
//       func->SetNormalizeVector(false);
//       // Stop the integration because the point is out of bounds.
//       if (out)
//         {
//         retVal=out;
//         memcpy(lastPoint,point2,3*sizeof(double));
//         break; // stop integrating
//         }
//       /// This has to change because it may be out side of the 
//       /// interpolator but we may need to continue our integration
//       /// loading the next neighborhood.
// 
// 
//       // It is not enough to use the starting point for stagnation calculation
//       // Use delX/stepSize to calculate speed and check if it is below
//       // stagnation threshold
//       double disp[3];
//       for (i=0; i<3; i++)
//         {
//         disp[i] = point2[i] - point1[i];
//         }
//       if ( (stepSize.Interval == 0) ||
//            (vtkMath::Norm(disp) / fabs(stepSize.Interval) <= this->TerminalSpeed) )
//         {
//         retVal = STAGNATION;
//         break;
//         }
//       // Calculate propagation (using the same units as MaximumPropagation
//       propagation+=fabs(stepTaken);
//       accumTime+=stepTaken/speed;
// 
// 
//       /// Prep for next step.
//       // This is the next starting point
//       for(i=0; i<3; i++)
//         {
//         point1[i] = point2[i];
//         }
// 
//       // Interpolate the velocity at the next point
//       if (!func->FunctionValues(point2, velocity))
//         {
//         retVal = OUT_OF_DOMAIN;
//         memcpy(lastPoint, point2, 3*sizeof(double));
//         break;
//         }
// 
//       // Make sure we use the dataset found by the vtkInterpolatedVelocityField
//       // TODO does this really change during line integration??
//       input = func->GetLastDataSet();
//       inputPD = input->GetPointData();
//       inVectors = inputPD->GetVectors(vecName);
// 
//       // Point is valid. Insert it.
//       numPts++;
//       numPtsTotal++;
//       nextPoint=outputPoints->InsertNextPoint(point1);
// 
// 
//       // Calculate cell length and speed to be used in unit conversions
//       input->GetCell(func->GetLastCellId(),cell);
//       cellLength=sqrt(static_cast<double>(cell->GetLength2()));
//       speed=vtkMath::Norm(velocity);
// 
//       // Interpolate all point attributes on current point
//       func->GetLastWeights(weights);
//       outputPD->InterpolatePoint(inputPD,nextPoint,cell->PointIds,weights);
// 
//       // Compute vorticity if required
//       // This can be used later for streamribbon generation.
//       if (this->ComputeVorticity)
//         {
//         time->InsertNextValue(accumTime);
//         //
//         inVectors->GetTuples(cell->PointIds, cellVectors);
//         func->GetLastLocalCoordinates(pcoords);
//         vtkOOCFieldTracer::CalculateVorticity(cell,pcoords,cellVectors,vort);
//         vorticity->InsertNextTuple(vort);
//         // rotation
//         // angular velocity = vorticity . unit tangent ( i.e. velocity/speed )
//         // rotation = sum ( angular velocity * stepSize )
//         omega = vtkMath::Dot(vort, velocity);
//         omega /= speed;
//         omega *= this->RotationScale;
//         index = angularVel->InsertNextValue(omega);
//         rotation->InsertNextValue(rotation->GetValue(index-1) +
//                                   (angularVel->GetValue(index-1) + omega)/2 * 
//                                   (accumTime - time->GetValue(index-1)));
//         }
//       // Add seed point id. This can be used to select line
//       // as a unit, and for reverse lookup, given the line
//       // look up the seed point.
//       vtkIdType seedPointId=seedIds->GetId(currentLine);
//       seedPointIds->InsertNextValue(seedPointId);
// 
//       // Never call conversion methods if speed == 0
//       if ( (speed == 0) || (speed <= this->TerminalSpeed) )
//         {
//         retVal = STAGNATION;
//         break;
//         }
// 
//       // Convert all intervals to arc length
//       this->ConvertIntervals( step, minStep, maxStep, direction, cellLength );
// 
// 
//       // If the solver is adaptive and the next step size (stepSize.Interval)
//       // that the solver wants to use is smaller than minStep or larger 
//       // than maxStep, re-adjust it. This has to be done every step
//       // because minStep and maxStep can change depending on the cell
//       // size (unless it is specified in arc-length unit)
//       if (integrator->IsAdaptive())
//         {
//         if (fabs(stepSize.Interval) < fabs(minStep))
//           {
//           stepSize.Interval = fabs( minStep ) * 
//                                 stepSize.Interval / fabs( stepSize.Interval );
//           }
//         else if (fabs(stepSize.Interval) > fabs(maxStep))
//           {
//           stepSize.Interval = fabs( maxStep ) * 
//                                 stepSize.Interval / fabs( stepSize.Interval );
//           }
//         }
//       else
//         {
//         stepSize.Interval = step;
//         }
//       } /// End Integration
// 
//     if (shouldAbort)
//       {
//       break;
//       }
// 
//     if (numPts > 1)
//       {
//       outputLines->InsertNextCell(numPts);
//       for (i=numPtsTotal-numPts; i<numPtsTotal; i++)
//         {
//         outputLines->InsertCellPoint(i);
//         }
//       retVals->InsertNextValue(retVal);
//       }
// 
//     // Initialize these to 0 before starting the next line.
//     // The values passed in the function call are only used
//     // for the first line.
//     inPropagation = propagation;
//     inNumSteps = numSteps;
// 
//     propagation = 0;
//     numSteps = 0;
//     }
// 
//   if (!shouldAbort)
//     {
//     // Create the output polyline
//     output->SetPoints(outputPoints);
//     outputPD->AddArray(time);
//     if (vorticity)
//       {
//       outputPD->AddArray(vorticity);
//       outputPD->AddArray(rotation);
//       outputPD->AddArray(angularVel);
//       }
// 
//     outputPD->AddArray(seedPointIds);
// 
//     vtkIdType numPts = outputPoints->GetNumberOfPoints();
//     if ( numPts > 1 )
//       {
//       // Assign geometry and attributes
//       output->SetLines(outputLines);
//       if (this->GenerateNormalsInIntegrate)
//         {
//         this->GenerateNormals(output, 0, vecName);
//         }
// 
//       outputCD->AddArray(retVals);
//       }
//     }
// 
//   if (vorticity)
//     {
//     vorticity->Delete();
//     rotation->Delete();
//     angularVel->Delete();
//     }
// 
//   seedPointIds->Delete();
// 
//   if (cellVectors)
//     {
//     cellVectors->Delete();
//     }
//   retVals->Delete();
// 
//   outputPoints->Delete();
//   outputLines->Delete();
// 
//   time->Delete();
// 
// 
//   integrator->Delete();
//   cell->Delete();
// 
//   delete[] weights;
//   
//   output->Squeeze();
//   return;
// }
