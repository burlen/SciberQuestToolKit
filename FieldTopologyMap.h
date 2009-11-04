/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef FieldTopologyMap_h
#define FieldTopologyMap_h

#include "FieldLine.h"

#include "vtkIntArray.h"
#include <vector>
using std::vector;

class CellIdBlock;
class FieldLine;
class vtkDataSet;
class vtkIntArray;
class TerminationCondition;

/// Interface to the topology map.
/**
Abstract collection of datastructures needed to build the topology map.
The details of building the map change drastically depending on the input
data type. Concrete classes deal with these specifics.
*/
class FieldTopologyMap
{
public:
  FieldTopologyMap();
  virtual ~FieldTopologyMap();

  // Description:
  // Set the datast to be used as the seed source.
  virtual void SetSource(vtkDataSet *s)=0;

  // Description:
  // Copy the IntersectColor and SourceId array into the output.
  virtual void SetOutput(vtkDataSet *o);

  // Description:
  // compute seed points (centred on cells of input). Copy the cells
  // on which we operate into the output.
  virtual int InsertCells(CellIdBlock *SourceIds)=0;


  // Description:
  // Get a specific field line.
  FieldLine *GetFieldLine(int i){ return this->Lines[i]; }

  // Description:
  // Free resources holding the trace geometry. This can be quite large.
  // Other data is retained.
  void ClearTrace(int i)
    {
    this->Lines[i]->DeleteTrace();
    }

  // Description:
  // Free internal resources.
  void ClearFieldLines();

  // Description:
  // Move scalar data (IntersectColor, SourceId) from the internal 
  // structure into the vtk output data.
  virtual int SyncScalars();

  // Description:
  // Move field line geometry (trace) from the internal structure
  // into the vtk output data.
  virtual int SyncGeometry(){ return 0; }

  // Description:
  // Access to the termination object.
  TerminationCondition *GetTerminationCondition(){ return this->Tcon; }

  // Description:
  // Print a legend, can be reduced to the minimal number of colors needed
  // or all posibilities may be included. The latter is better for temporal
  // animations.
  void PrintLegend(int reduce);

protected:
  int *Append(vtkIntArray *ia, int nn);

protected:
  vtkIntArray *IntersectColor;
  int *pIntersectColor;
  vtkIntArray *SourceId;
  int *pSourceId;
  vector<FieldLine *> Lines;
  TerminationCondition *Tcon;
};

#endif
