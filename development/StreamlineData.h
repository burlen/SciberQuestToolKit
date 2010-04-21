/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef StreamlineData_h
#define StreamlineData_h

#include "FieldTraceData.h"

#include<vector>
using std::vector;

#include<map>
using std::map;
using std::pair;

class CellIdBlock;
class vtkDataSet;
class vtkFloatArray;
class vtkCellArray;
class vtkUnsignedCharArray;
class vtkIdTypeArray;

/// Interface to the topology map.
/**
Abstract collection of datastructures needed to build the topology map.
The details of building the map change drastically depending on the input
data type. Concrete classes deal with these specifics.
*/
class StreamlineData : public FieldTraceData
{
public:
  StreamlineData()
        :
    SourcePts(0),
    SourceCells(0),
    OutPts(0),
    OutCells(0)
      {  }

  virtual ~StreamlineData();

  // Description:
  // Set the datast to be used as the seed source.
  virtual void SetSource(vtkDataSet *s);

  // Description:
  // Copy the IntersectColor and SourceId array into the output.
  virtual void SetOutput(vtkDataSet *o);

  // Description:
  // Convert a list of seed cells (sourceIds) to FieldLine
  // structures and build the output (if any).
  virtual int InsertCells(CellIdBlock *SourceIds);

  // Description:
  // Move field line geometry (trace) from the internal structure
  // into the vtk output data.
  virtual int SyncGeometry();


private:
  void ClearSource();
  void ClearOut();

private:
  vtkFloatArray *SourcePts;
  vtkCellArray *SourceCells;

  vtkFloatArray *OutPts;
  vtkCellArray *OutCells;
};

#endif
