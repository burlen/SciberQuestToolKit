/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "FieldTracerData.h"

#include "WorkQueue.h"
#include "TerminationCondition.h"
#include "vtkDataSet.h"
#include "vtkCellData.h"

//-----------------------------------------------------------------------------
FieldTracerData::FieldTracerData()
      :
    IntersectColor(0)//,
    //SourceId(0)
{
  this->IntersectColor=vtkIntArray::New();
  this->IntersectColor->SetName("IntersectColor");

  // this->SourceId=vtkIntArray::New();
  // this->SourceId->SetName("SourceId");

  this->Tcon=new TerminationCondition;
}

//-----------------------------------------------------------------------------
FieldTracerData::~FieldTracerData()
{
  this->IntersectColor->Delete();
  // this->SourceId->Delete();
  this->ClearFieldLines();

  delete this->Tcon;
}

//-----------------------------------------------------------------------------
void FieldTracerData::SetOutput(vtkDataSet *o)
{
  o->GetCellData()->AddArray(this->IntersectColor);
  // o->GetCellData()->AddArray(this->SourceId);
}

//-----------------------------------------------------------------------------
int *FieldTracerData::Append(vtkIntArray *ia, int nn)
{
  vtkIdType ne=ia->GetNumberOfTuples();
  return ia->WritePointer(ne,nn);
}

//-----------------------------------------------------------------------------
void FieldTracerData::ClearFieldLines()
{
  size_t nLines=this->Lines.size();
  for (size_t i=0; i<nLines; ++i)
    {
    delete this->Lines[i];
    }
  this->Lines.clear();
}

//-----------------------------------------------------------------------------
int FieldTracerData::SyncScalars()
{
  vtkIdType nLines=this->Lines.size();

  vtkIdType lastLineId=this->IntersectColor->GetNumberOfTuples();

  int *pColor=this->IntersectColor->WritePointer(lastLineId,nLines);
  // int *pId=this->SourceId->WritePointer(lastLineId,nLines);

  for (vtkIdType i=0; i<nLines; ++i)
    {
    FieldLine *line=this->Lines[i];

    *pColor=this->Tcon->GetTerminationColor(line);
    ++pColor;

    // *pId=line->GetSeedId();
    // ++pId;
    }
  return 1;
}

//-----------------------------------------------------------------------------
void FieldTracerData::PrintLegend(int reduce)
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
