/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "FieldTopologyMap.h"

#include "CellIdBlock.h"
#include "Fieldline.h"
#include "vtkDataSet.h"
#include "vtkCellData.h"

//-----------------------------------------------------------------------------
FieldTopologyMap::FieldTopologyMap()
      :
    IntersectColor(0),
    SourceId(0)
{
  this->IntersectColor=vtkIntArray::New();
  this->IntersectColor->SetName("IntersectColor");

  this->SourceId=vtkIntArray::New();
  this->SourceId->SetName("SourceId");
}

//-----------------------------------------------------------------------------
FieldTopologyMap::~FieldTopologyMap()
{
  this->IntersectColor->Delete();
  this->SourceId->Delete();
}

//-----------------------------------------------------------------------------
void FieldTopologyMap::SetOutput(vtkDataSet *o)
{
  o->GetCellData()->AddArray(this->IntersectColor);
  o->GetCellData()->AddArray(this->SourceId);
}

//-----------------------------------------------------------------------------
int *FieldTopologyMap::Append(vtkIntArray *ia, int nn)
{
  vtkIdType ne=ia->GetNumberOfTuples();
  return ia->WritePointer(ne,nn);
}

//-----------------------------------------------------------------------------
void FieldTopologyMap::ClearFieldLines()
{
  size_t nLines=this->Lines.size();
  for (size_t i=0; i<nLines; ++i)
    {
    delete this->Lines[i];
    }
  this->Lines.clear();
}

//-----------------------------------------------------------------------------
void FieldTopologyMap::SyncScalars()
{
  vtkIdType nLines=this->Lines.size();

  vtkIdType lastLineId=this->IntersectColor->GetNumberOfTuples();

  int *pColor=intersectColor->WritePointer(lastLineId,nLines);
  int *pId=sourceId->WritePointer(lastLineId,nLines);

  for (vtkIdType i=0; i<nLines; ++i)
    {
    FieldLine *line=this->Lines[i];

    *pColor=this->Tcon->GetTerminationColor(line);
    ++pColor;

    *pId=line->GetSeedId();
    ++pId;
    }
}
