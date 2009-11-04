/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef PolyDataFieldTopologyMap_h
#define PolyDataFieldTopologyMap_h

#include<vector>
using std::vector;

#include<map>
using std::map;
using std::pair;

class CellIdBlock;
class FieldLine;
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
class PolyDataFieldTopologyMap
{
public:
  PolyDataFieldTopologyMap()
        :
    SourcePts(0),
    SourceCells(0),
    OutPts(0),
    OutCells(0),
    CellType(NONE)
      {  }

  virtual ~PolyDataFieldTopologyMap();

  // Description:
  // Set the datast to be used as the seed source.
  virtual void SetSource(vtkDataSet *s);

  // Description:
  // Copy the IntersectColor and SourceId array into the output.
  virtual void SetOutput(vtkDataSet *o);

  // Description:
  // Convert a list of seed cells (sourceIds) to FieldLine
  // structures and build the output (if any).
  virtual int InsertCells(
        CellIdBlock *SourceIds,
        vector<FieldLine *> &lines);


private:
  void ClearSource();
  void ClearOut();

private:
  typedef pair<map<vtkIdType,vtkIdType>::iterator,bool> MapInsert;
  typedef pair<vtkIdType,vtkIdType> MapElement;
  map<vtkIdType,vtkIdType> IdMap;

  vtkFloatArray *SourcePts;
  vtkCellArray *SourceCells;

  vtkFloatArray *OutPts;
  vtkCellArray *OutCells;

  enum {NONE=0,POLY=1,VERT=2};
  int CellType;
};

#endif
