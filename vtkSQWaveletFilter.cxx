/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2012 SciberQuest Inc.

*/
#include "vtkSQWaveletFilter.h"

// #define vtkSQWaveletFilterDEBUG
// #define vtkSQWaveletFilterTIME

#if defined vtkSQWaveletFilterTIME
  #include "vtkSQLog.h"
#endif

#include "Tuple.hxx"
#include "CartesianExtent.h"
#include "CartesianExtent.h"
#include "postream.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
typedef vtkStreamingDemandDrivenPipeline vtkSDDPipeline;

#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include <vtkstd/string>
using vtkstd::string;
#include <vtkstd/map>
using vtkstd::map;
#include <vtkstd/vector>
using vtkstd::vector;
#include <vtkstd/utility>
using vtkstd::pair;
#include <vtkstd/algorithm>
using vtkstd::min;
using vtkstd::max;

#ifndef SQTK_WITHOUT_MPI
#include <mpi.h>
#endif

vtkCxxRevisionMacro(vtkSQWaveletFilter, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkSQWaveletFilter);

//-----------------------------------------------------------------------------
vtkSQWaveletFilter::vtkSQWaveletFilter()
    :
  WorldSize(1),
  WorldRank(0),
  WaveletBasis(WAVELET_BASIS_NONE),
  Invert(0),
  Mode(CartesianExtent::DIM_MODE_3D),
  NumberOfGhostCells(0)
{
  #ifdef vtkSQWaveletFilterDEBUG
  pCerr() << "=====vtkSQWaveletFilter::vtkSQWaveletFilter" << endl;
  #endif

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->SetWaveletBasis(WAVELET_BASIS_HAAR);

  #ifndef SQTK_WITHOUT_MPI
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (mpiOk)
    {
    MPI_Comm_rank(MPI_COMM_WORLD,&this->WorldRank);
    MPI_Comm_size(MPI_COMM_WORLD,&this->WorldSize);
    }
  #endif

  #ifdef vtkSQWaveletFilterDEBUG
  pCerr() << "HostSize=" << this->HostSize << endl;
  pCerr() << "HostRank=" << this->HostRank << endl;
  #endif
}

//-----------------------------------------------------------------------------
vtkSQWaveletFilter::~vtkSQWaveletFilter()
{
  #ifdef vtkSQWaveletFilterDEBUG
  pCerr() << "=====vtkSQWaveletFilter::~vtkSQWaveletFilter" << endl;
  #endif

  this->SetWaveletBasis(WAVELET_BASIS_NONE);
}

//-----------------------------------------------------------------------------
void vtkSQWaveletFilter::AddInputArray(const char *name)
{
  #ifdef vtkSQWaveletFilterDEBUG
  pCerr()
    << "=====vtkSQWaveletFilter::AddInputArray"
    << "name=" << name << endl;
  #endif

  if (this->InputArrays.insert(name).second)
    {
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSQWaveletFilter::ClearInputArrays()
{
  #ifdef vtkSQWaveletFilterDEBUG
  pCerr()
    << "=====vtkSQWaveletFilter::ClearInputArrays" << endl;
  #endif

  if (this->InputArrays.size())
    {
    this->InputArrays.clear();
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSQWaveletFilter::SetWaveletBasis(int basis)
{
  #ifdef vtkSQWaveletFilterDEBUG
  pCerr()
    << "=====vtkSQWaveletFilter::SetWaveletBasis"
    << "basis=" << basis << endl;
  #endif

  if (this->WaveletBasis==basis)
    {
    return;
    }

  this->WaveletBasis=basis;

  // Joerg:
  // Set number of ghost cells to request here.
  // Allocate basis dependent data (free previous allocations)
  switch (basis)
    {
    case WAVELET_BASIS_NONE:
      break;

    case WAVELET_BASIS_HAAR:
      this->SetNumberOfGhostCells(0);
      break;

    default:
      vtkErrorMacro("Invalid wavelet basis " << basis << " requested.");
    };

  this->Modified();
}
//-----------------------------------------------------------------------------
int vtkSQWaveletFilter::RequestDataObject(
    vtkInformation* /* request */,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec)
{
  #ifdef vtkSQWaveletFilterDEBUG
  pCerr() << "=====vtkSQWaveletFilter::RequestDataObject" << endl;
  #endif


  vtkInformation *inInfo=inInfoVec[0]->GetInformationObject(0);
  vtkDataObject *inData=inInfo->Get(vtkDataObject::DATA_OBJECT());
  const char *inputType=inData->GetClassName();

  vtkInformation *outInfo=outInfoVec->GetInformationObject(0);
  vtkDataObject *outData=outInfo->Get(vtkDataObject::DATA_OBJECT());

  if ( !outData || !outData->IsA(inputType))
    {
    outData=inData->NewInstance();
    outInfo->Set(vtkDataObject::DATA_TYPE_NAME(),inputType);
    outInfo->Set(vtkDataObject::DATA_OBJECT(),outData);
    outInfo->Set(vtkDataObject::DATA_EXTENT_TYPE(), inData->GetExtentType());
    outData->SetPipelineInformation(outInfo);
    outData->Delete();
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQWaveletFilter::RequestInformation(
      vtkInformation * /*req*/,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQWaveletFilterDEBUG
  pCerr() << "=====vtkSQWaveletFilter::RequestInformation" << endl;
  #endif

  // We will work in a restricted problem domain so that we have
  // always a single layer of ghost cells available. To make it so
  // we'll take the upstream's domain and shrink it by the requested
  // number of ghost cells.

  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);
  CartesianExtent inputDomain;
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        inputDomain.GetData());

  // determine the dimensionality of the input.
  this->Mode
    = CartesianExtent::GetDimensionMode(
          inputDomain,
          this->NumberOfGhostCells);
  if (this->Mode==CartesianExtent::DIM_MODE_INVALID)
    {
    vtkErrorMacro("Invalid problem domain.");
    }

  // shrink the output problem domain by the requisite number of
  // ghost cells.
  CartesianExtent outputDomain
    = CartesianExtent::Grow(
          inputDomain,
          -this->NumberOfGhostCells,
          this->Mode);

  vtkInformation* outInfo=outInfos->GetInformationObject(0);
  outInfo->Set(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        outputDomain.GetData(),
        6);

  // other keys that need to be coppied
  double dX[3];
  inInfo->Get(vtkDataObject::SPACING(),dX);
  outInfo->Set(vtkDataObject::SPACING(),dX,3);

  double X0[3];
  inInfo->Get(vtkDataObject::ORIGIN(),X0);
  outInfo->Set(vtkDataObject::ORIGIN(),X0,3);

  #ifdef vtkSQWaveletFilterDEBUG
  pCerr()
    << "WHOLE_EXTENT(input)=" << inputDomain << endl
    << "WHOLE_EXTENT(output)=" << outputDomain << endl
    << "ORIGIN=" << Tuple<double>(X0,3) << endl
    << "SPACING=" << Tuple<double>(dX,3) << endl
    << "nGhost=" << this->NumberOfGhostCells << endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQWaveletFilter::RequestUpdateExtent(
      vtkInformation *req,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQWaveletFilterDEBUG
  pCerr() << "=====vtkSQWaveletFilter::RequestUpdateExtent" << endl;
  #endif

  // if no ghost cells are needed then we could just
  // do the standard request.
  if (this->NumberOfGhostCells<1)
    {
    return Superclass::RequestUpdateExtent(req,inInfos,outInfos);
    }


  vtkInformation* outInfo=outInfos->GetInformationObject(0);
  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);

  // We will modify the extents we request from our input so
  // that we will have a layers of ghost cells. We also pass
  // the number of ghosts through the piece based key.
  inInfo->Set(
        vtkSDDPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
        this->NumberOfGhostCells);

  CartesianExtent outputExt;
  outInfo->Get(
        vtkSDDPipeline::UPDATE_EXTENT(),
        outputExt.GetData());

  CartesianExtent wholeExt;
  inInfo->Get(
        vtkSDDPipeline::WHOLE_EXTENT(),
        wholeExt.GetData());

  outputExt = CartesianExtent::Grow(
        outputExt,
        wholeExt,
        this->NumberOfGhostCells,
        this->Mode);

  inInfo->Set(
        vtkSDDPipeline::UPDATE_EXTENT(),
        outputExt.GetData(),
        6);

  int piece
    = outInfo->Get(vtkSDDPipeline::UPDATE_PIECE_NUMBER());

  int numPieces
    = outInfo->Get(vtkSDDPipeline::UPDATE_NUMBER_OF_PIECES());

  inInfo->Set(vtkSDDPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkSDDPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(vtkSDDPipeline::EXACT_EXTENT(), 1);

  #ifdef vtkSQWaveletFilterDEBUG
  pCerr()
    << "WHOLE_EXTENT=" << wholeExt << endl
    << "UPDATE_EXTENT=" << outputExt << endl
    << "this->NumberOfGhostCells=" << this->NumberOfGhostCells << endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQWaveletFilter::RequestData(
    vtkInformation * /*req*/,
    vtkInformationVector **inInfoVec,
    vtkInformationVector *outInfoVec)
{
  #ifdef vtkSQWaveletFilterDEBUG
  pCerr() << "=====vtkSQWaveletFilter::RequestData" << endl;
  #endif

  #if defined vtkSQWaveletFilterTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("vtkSQWaveletFilter::RequestData");
  #endif

  vtkInformation *inInfo=inInfoVec[0]->GetInformationObject(0);
  vtkDataObject *inData=inInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkInformation *outInfo=outInfoVec->GetInformationObject(0);
  vtkDataObject *outData=outInfo->Get(vtkDataObject::DATA_OBJECT());

  // Guard against empty input.
  if (!inData || !outData)
    {
    vtkErrorMacro(
      << "Empty input(" << inData << ") or output(" << outData << ") detected.");
    return 1;
    }
  // We need extent based data here.
  int isImage=inData->IsA("vtkImageData");
  int isRecti=inData->IsA("vtkrectilinearGrid");
  if (!isImage && !isRecti)
    {
    vtkErrorMacro(
      << "This filter is designed for vtkStructuredData and subclasses."
      << "You are trying to use it with " << inData->GetClassName() << ".");
    return 1;
    }

  // Get the input and output extents.
  CartesianExtent inputExt;
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        inputExt.GetData());

  CartesianExtent inputDom;
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        inputDom.GetData());

  CartesianExtent outputExt;
  outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        outputExt.GetData());

  CartesianExtent domainExt;
  outInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        domainExt.GetData());

  // Check that we have the ghost cells that we need (more is OK).
  CartesianExtent inputBox(inputExt);
  CartesianExtent outputBox
    = CartesianExtent::Grow(outputExt, this->NumberOfGhostCells, this->Mode);

  if (!inputBox.Contains(outputBox))
    {
    vtkErrorMacro(
      << "This filter requires ghost cells to function correctly. "
      << "The input must conatin the output plus " << this->NumberOfGhostCells
      << " layers of ghosts. The input is " << inputBox
      << ", but it must be at least "
      << outputBox << ".");
    return 1;
    }

  // NOTE You can't do a shallow copy because the array dimensions are
  // different on output and input because of the ghost layers.

  if (isImage)
    {
    vtkImageData *inImData=dynamic_cast<vtkImageData *>(inData);
    vtkImageData *outImData=dynamic_cast<vtkImageData *>(outData);

    // set up the output.
    double X0[3];
    outInfo->Get(vtkDataObject::ORIGIN(),X0);
    outImData->SetOrigin(X0);

    double dX[3];
    outInfo->Get(vtkDataObject::SPACING(),dX);
    outImData->SetSpacing(dX);

    outImData->SetExtent(outputExt.GetData());

    int outputDims[3];
    outImData->GetDimensions(outputDims);
    int outputTups=outputDims[0]*outputDims[1]*outputDims[2];

    #ifdef vtkSQWaveletFilterDEBUG
    pCerr()
      << "WHOLE_EXTENT=" << domainExt << endl
      << "UPDATE_EXTENT(input)=" << inputExt << endl
      << "UPDATE_EXTENT(output)=" << outputExt << endl
      << "ORIGIN" << Tuple<double>(X0,3) << endl
      << "SPACING" << Tuple<double>(dX,3) << endl
      << endl;
    #endif

    set<string>::iterator it;
    set<string>::iterator begin=this->InputArrays.begin();
    set<string>::iterator end=this->InputArrays.end();
    for (it=begin; it!=end; ++it)
      {
      vtkDataArray *V=inImData->GetPointData()->GetArray((*it).c_str());
      if (V==0)
        {
        vtkErrorMacro(
          << "Array " << (*it).c_str()
          << " was requested but is not present");
        continue;
        }

      if (!V->IsA("vtkFloatArray") && !V->IsA("vtkDoubleArray"))
        {
        vtkErrorMacro(
          << "This filter operates on vector floating point arrays."
          << "You provided " << V->GetClassName() << ".");
        return 1;
        }

      int nComps = V->GetNumberOfComponents();

      vtkDataArray *W=V->NewInstance();
      W->SetNumberOfComponents(nComps);
      W->SetNumberOfTuples(outputTups);
      W->SetName(V->GetName());

      // Joerg:
      // call your function here as follows:
      if (this->Invert)
        {
        /*
        switch (V->GetDataType())
          {
          vtkTemplateMacro(
            waveletInverseTransform<VTK_TT>(
                extV.GetData(),
                extW.GetData(),
                extK.GetData(),
                nComp,
                mode,
                (VTK_TT*)V->GetVoidPointer(0),
                (VTK_TT*)W->GetVoidPointer(0))
                );
          }
        */
        }
      else
        {
        /*
        switch (V->GetDataType())
          {
          vtkTemplateMacro(
            waveletTransform<VTK_TT>(
                extV.GetData(),
                extW.GetData(),
                extK.GetData(),
                nComp,
                mode,
                (VTK_TT*)V->GetVoidPointer(0),
                (VTK_TT*)W->GetVoidPointer(0))
                );
          }
        */
        }

      outImData->GetPointData()->AddArray(W);
      W->Delete();
      }

    // outImData->Print(cerr);
    }
  else
  if (isRecti)
    {
    vtkWarningMacro("TODO : implment difference opperators on stretched grids.");
    }

  #if defined vtkSQWaveletFilterTIME
  log->EndEvent("vtkSQWaveletFilter::RequestData");
  #endif

 return 1;
}

//-----------------------------------------------------------------------------
void vtkSQWaveletFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef vtkSQWaveletFilterDEBUG
  pCerr() << "=====vtkSQWaveletFilter::PrintSelf" << endl;
  #endif

  this->Superclass::PrintSelf(os,indent);
}
