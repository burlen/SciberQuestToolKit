/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef FieldDisplacementMapData_h
#define FieldDisplacementMapData_h

#include "FieldTraceData.h"

class vtkDataSet;
class vtkFloatArray;

/// Interface to the topology map.
/**
Abstract collection of datastructures needed to build the topology map.
The details of building the map change drastically depending on the input
data type. Concrete classes deal with these specifics.
*/
class FieldDisplacementMapData : public FieldTraceData
{
public:
  FieldDisplacementMapData();
  virtual ~FieldDisplacementMapData();

  /**
  Copy the IntersectColor and SourceId array into the output.
  */
  virtual void SetOutput(vtkDataSet *o);

  /**
  Move scalar data (IntersectColor, SourceId) from the internal
  structure into the vtk output data.
  */
  virtual int SyncScalars();

  /**
  Move field line geometry (trace) from the internal structure
  into the vtk output data.
  */
  virtual int SyncGeometry(){ return 0; }

protected:
  vtkFloatArray *Displacement;
  vtkFloatArray *FwdDisplacement;
  vtkFloatArray *BwdDisplacement;
};

#endif
