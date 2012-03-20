/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "FieldTraceData.h"

#include "TerminationCondition.h"
#include "vtkDataSet.h"
#include "vtkCellData.h"

//-----------------------------------------------------------------------------
FieldTraceData::FieldTraceData()
        :
    IntersectColor(0),
    Displacement(0),
    FwdDisplacement(0),
    BwdDisplacement(0)
    //SourceId(0)
{
  this->IntersectColor=vtkIntArray::New();
  this->IntersectColor->SetName("IntersectColor");

  // this->SourceId=vtkIntArray::New();
  // this->SourceId->SetName("SourceId");

  this->Tcon=new TerminationCondition;
}

//-----------------------------------------------------------------------------
FieldTraceData::~FieldTraceData()
{
  this->IntersectColor->Delete();
  // this->SourceId->Delete();

  this->SetComputeDisplacementMap(0);

  this->ClearFieldLines();

  delete this->Tcon;
}

//-----------------------------------------------------------------------------
void FieldTraceData::SetComputeDisplacementMap(int enable)
{
  if (this->Displacement)
    {
    this->Displacement->Delete();
    this->Displacement=0;
    }

  if (this->FwdDisplacement)
    {
    this->FwdDisplacement->Delete();
    this->FwdDisplacement=0;
    }

  if (this->BwdDisplacement)
    {
    this->BwdDisplacement->Delete();
    this->BwdDisplacement=0;
    }

  if (enable)
    {
    this->Displacement=vtkFloatArray::New();
    this->Displacement->SetName("displacement");
    this->Displacement->SetNumberOfComponents(3);

    this->FwdDisplacement=vtkFloatArray::New();
    this->FwdDisplacement->SetName("fwd-displacement-map");
    this->FwdDisplacement->SetNumberOfComponents(3);

    this->BwdDisplacement=vtkFloatArray::New();
    this->BwdDisplacement->SetName("bwd-displacement-map");
    this->BwdDisplacement->SetNumberOfComponents(3);
    }
}

//-----------------------------------------------------------------------------
void FieldTraceData::SetOutput(vtkDataSet *o)
{
  o->GetCellData()->AddArray(this->IntersectColor);

  if (this->Displacement)
    {
    o->GetCellData()->AddArray(this->Displacement);
    }

  if (this->FwdDisplacement)
    {
    o->GetCellData()->AddArray(this->FwdDisplacement);
    }

  if (this->BwdDisplacement)
    {
    o->GetCellData()->AddArray(this->BwdDisplacement);
    }

  // o->GetCellData()->AddArray(this->SourceId);
}

//-----------------------------------------------------------------------------
int *FieldTraceData::Append(vtkIntArray *ia, int nn)
{
  vtkIdType ne=ia->GetNumberOfTuples();
  return ia->WritePointer(ne,nn);
}

//-----------------------------------------------------------------------------
void FieldTraceData::ClearFieldLines()
{
  size_t nLines=this->Lines.size();
  for (size_t i=0; i<nLines; ++i)
    {
    delete this->Lines[i];
    }
  this->Lines.clear();
}

//-----------------------------------------------------------------------------
int FieldTraceData::SyncScalars()
{
  vtkIdType nLines=this->Lines.size();

  vtkIdType lastLineId=this->IntersectColor->GetNumberOfTuples();

  int *pColor=this->IntersectColor->WritePointer(lastLineId,nLines);

  float *pDisplacement=0;
  if (this->Displacement)
    {
    pDisplacement
      = this->Displacement->WritePointer(3*lastLineId,3*nLines);
    }

  float *pFwdDisplacement=0;
  if (this->FwdDisplacement)
    {
    pFwdDisplacement
      = this->FwdDisplacement->WritePointer(3*lastLineId,3*nLines);
    }

  float *pBwdDisplacement=0;
  if (this->BwdDisplacement)
    {
    pBwdDisplacement
      = this->BwdDisplacement->WritePointer(3*lastLineId,3*nLines);
    }

  // int *pId=this->SourceId->WritePointer(lastLineId,nLines);

  for (vtkIdType i=0; i<nLines; ++i)
    {
    FieldLine *line=this->Lines[i];

    *pColor=this->Tcon->GetTerminationColor(line);
    pColor+=1;

    if (pDisplacement)
      {
      line->GetDisplacement(pDisplacement);
      pDisplacement+=3;
      }

    if (pFwdDisplacement)
      {
      line->GetForwardEndPoint(pFwdDisplacement);
      pFwdDisplacement+=3;
      }

    if (pBwdDisplacement)
      {
      line->GetBackwardEndPoint(pBwdDisplacement);
      pBwdDisplacement+=3;
      }

    // *pId=line->GetSeedId();
    // ++pId;
    }
  return 1;
}

//-----------------------------------------------------------------------------
void FieldTraceData::PrintLegend(int reduce)
{
  if (reduce)
    {
    this->Tcon->SqueezeColorMap(this->IntersectColor);
    }
  else
    {
    this->Tcon->PrintColorMap();
    }
}
