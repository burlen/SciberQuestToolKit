/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQFieldTopologySplit.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQFieldTopologySplit.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include "vtkUnstructuredGrid.h"
#include "vtkPolyData.h"
#include "vtkCellData.h"
#include "vtkAppendFilter.h"
#include "vtkThreshold.h"


//=============================================================================
class TopologicalClassSelector
{
public:
  TopologicalClassSelector();
  ~TopologicalClassSelector();

  void Initialize();
  void Clear();

  void SetInput(vtkDataSet *input);
  vtkUnstructuredGrid *GetOutput();

  void AppendRange(double v0, double v1);

private:
  vtkDataSet *Input;
  vtkThreshold *Threshold;
  vtkAppendFilter *Append;
};

//-----------------------------------------------------------------------------
TopologicalClassSelector::TopologicalClassSelector()
    :
  Input(0),
  Threshold(0),
  Append(0)
{
  this->Initialize();
}

//-----------------------------------------------------------------------------
TopologicalClassSelector::~TopologicalClassSelector()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
void TopologicalClassSelector::Initialize()
{
  this->Clear();
  this->Append=vtkAppendFilter::New();
}

//-----------------------------------------------------------------------------
void TopologicalClassSelector::Clear()
{
  if (this->Input)
    {
    this->Input->Delete();
    }

  if (this->Append)
    {
    this->Append->Delete();
    this->Append=0;
    }
}

//-----------------------------------------------------------------------------
void TopologicalClassSelector::SetInput(vtkDataSet *input)
{
  if (this->Input==input)
    {
    return;
    }

  if (this->Input)
    {
    this->Input->Delete();
    }

  this->Input=input;

  if (this->Input)
    {
    this->Input=input->NewInstance();
    this->Input->ShallowCopy(input);
    this->Input->GetCellData()->SetActiveScalars("IntersectColor");
    }
}

//-----------------------------------------------------------------------------
void TopologicalClassSelector::AppendRange(double v0, double v1)
{
  vtkThreshold *threshold=vtkThreshold::New();
  threshold->SetInput(this->Input);
  threshold->SetInputArrayToProcess(
        0,
        0,
        0,
        vtkDataObject::FIELD_ASSOCIATION_CELLS,
        "IntersectColor");
  threshold->ThresholdBetween(v0,v1);

  vtkUnstructuredGrid *ug=threshold->GetOutput();
  ug->Update();

  this->Append->AddInput(ug);

  threshold->Delete();
}

//-----------------------------------------------------------------------------
vtkUnstructuredGrid *TopologicalClassSelector::GetOutput()
{
  vtkUnstructuredGrid *ug=this->Append->GetOutput();
  ug->Update();

//   cerr << "Geting output" << endl;
//   ug->Print(cerr);

  return ug;
}

//=============================================================================

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSQFieldTopologySplit,"$Revision: 1.0$");

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQFieldTopologySplit);

//-----------------------------------------------------------------------------
vtkSQFieldTopologySplit::vtkSQFieldTopologySplit()
{
  #ifdef vtkSQFieldTopologySplitDEBUG
  cerr << "===============================vtkSQFieldTopologySplit::vtkSQFieldTopologySplit" << endl;
  #endif

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(5);
}

//-----------------------------------------------------------------------------
vtkSQFieldTopologySplit::~vtkSQFieldTopologySplit()
{
  #ifdef vtkSQFieldTopologySplitDEBUG
  cerr << "===============================vtkSQFieldTopologySplit::~vtkSQFieldTopologySplit" << endl;
  #endif
}

//----------------------------------------------------------------------------
int vtkSQFieldTopologySplit::FillInputPortInformation(
      int /*port*/,
      vtkInformation *info)
{
  #ifdef vtkSQFieldTopologySplitDEBUG
  cerr << "===============================vtkSQFieldTopologySplit::FillInputPortInformation" << endl;
  #endif

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(),0);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSQFieldTopologySplit::FillOutputPortInformation(
      int /*port*/,
      vtkInformation *info)
{
  #if vtkSQFieldTopologySplitDEBUG>1
  pCerr() << "===============================vtkSQFieldTopologySplit::FillOutputPortInformation" << endl;
  #endif

  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkSQFieldTopologySplit::RequestInformation(
    vtkInformation */*req*/,
    vtkInformationVector **/*inInfos*/,
    vtkInformationVector *outInfos)
{
  #ifdef vtkSQFieldTopologySplitDEBUG
    cerr << "===============================vtkSQFieldTopologySplit::RequestInformation" << endl;
  #endif


  // tell the excutive that we are handling our own decomposition.
  // once for each output port
  for (int p=0; p<5; ++p)
    {
    vtkInformation *outInfo=outInfos->GetInformationObject(p);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQFieldTopologySplit::RequestData(
      vtkInformation *vtkNotUsed(request),
      vtkInformationVector **inputVector,
      vtkInformationVector *outputVector)
{
  #ifdef vtkSQFieldTopologySplitDEBUG
  cerr << "===============================vtkSQFieldTopologySplit::RequestData" << endl;
  #endif

  vtkInformation *inInfo=inputVector[0]->GetInformationObject(0);

  vtkDataSet *input
    = dynamic_cast<vtkDataSet*>(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input==0)
    {
    vtkErrorMacro("Empty input.");
    return 1;
    }

  if ( (dynamic_cast<vtkPolyData*>(input)==0)
    && (dynamic_cast<vtkUnstructuredGrid*>(input)==0))
    {
    vtkErrorMacro("Input type " << input->GetClassName() << " is unsupported.");
    return 1;
    }

  vtkInformation *outInfo;
  vtkUnstructuredGrid *output;
  int pieceNo;
  int nPieces;
  int portNo=0;
  double progInc=0.2;

  //----------------------------------------
  //  class       value     definition
  //--------------------------------------
  //  solar wind  0         d-d
  //              3         0-d
  //              4         i-d

  // get the info object
  outInfo=outputVector->GetInformationObject(portNo);
  ++portNo;

  // get the ouptut
  output
    = dynamic_cast<vtkUnstructuredGrid*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if (pieceNo>=nPieces)
    {
    // sanity - the requst cannot be fullfilled
    output->Initialize();
    }
  else
    {
    // create this class
    TopologicalClassSelector sel;
    sel.SetInput(input);
    sel.AppendRange(-0.5,0.5);
    sel.AppendRange(2.5,4.5);
    output->ShallowCopy(sel.GetOutput());
    }
  this->UpdateProgress(portNo*progInc);

  //--------------------------------------
  //  magnetos-   5         n-n
  //  phere       6         s-n
  //              9         s-s

  // get the info object
  outInfo=outputVector->GetInformationObject(portNo);
  ++portNo;

  // get the ouptut
  output
    = dynamic_cast<vtkUnstructuredGrid*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if (pieceNo>=nPieces)
    {
    // sanity - the requst cannot be fullfilled
    output->Initialize();
    }
  else
    {
    // create this class
    TopologicalClassSelector sel;
    sel.SetInput(input);
    sel.AppendRange(4.5,6.5);
    sel.AppendRange(8.5,9.5);
    output->ShallowCopy(sel.GetOutput());
    }
  this->UpdateProgress(portNo*progInc);

  //--------------------------------------
  //  north       1         n-d
  //  connected   7         0-n
  //              8         i-n

  // get the info object
  outInfo=outputVector->GetInformationObject(portNo);
  ++portNo;

  // get the ouptut
  output
    = dynamic_cast<vtkUnstructuredGrid*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if (pieceNo>=nPieces)
    {
    // sanity - the requst cannot be fullfilled
    output->Initialize();
    }
  else
    {
    // create this class
    TopologicalClassSelector sel;
    sel.SetInput(input);
    sel.AppendRange(0.5,1.5);
    sel.AppendRange(6.5,8.5);
    output->ShallowCopy(sel.GetOutput());
    }
  this->UpdateProgress(portNo*progInc);

  //--------------------------------------
  //  south       2         s-d
  //  connected   10        0-s
  //              11        i-s

  // get the info object
  outInfo=outputVector->GetInformationObject(portNo);
  ++portNo;

  // get the ouptut
  output
    = dynamic_cast<vtkUnstructuredGrid*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if (pieceNo>=nPieces)
    {
    // sanity - the requst cannot be fullfilled
    output->Initialize();
    }
  else
    {
    // create this class
    TopologicalClassSelector sel;
    sel.SetInput(input);
    sel.AppendRange(1.5,2.5);
    sel.AppendRange(9.5,11.5);
    output->ShallowCopy(sel.GetOutput());
    }
  this->UpdateProgress(portNo*progInc);

  //-------------------------------------
  //  null/short  12        0-0
  //  integration 13        i-0
  //              14        i-i
  //---------------------------------------

  // get the info object
  outInfo=outputVector->GetInformationObject(portNo);
  ++portNo;

  // get the ouptut
  output
    = dynamic_cast<vtkUnstructuredGrid*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if (pieceNo>=nPieces)
    {
    // sanity - the requst cannot be fullfilled
    output->Initialize();
    }
  else
    {
    // create this class
    TopologicalClassSelector sel;
    sel.SetInput(input);
    sel.AppendRange(11.5,14.5);
    output->ShallowCopy(sel.GetOutput());
    }
  this->UpdateProgress(portNo*progInc);

  return 1;
}


//-----------------------------------------------------------------------------
void vtkSQFieldTopologySplit::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
