/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __vtkSQOnDemandSource_h
#define __vtkSQOnDemandSource_h

/// Interface to sources that provide data on demand
/**

*/
class vtkSQOnDemandSource : public vtkObject
{
public:
  /**
  Key that provides access to the source object.
  */
  static vtkInformationObjectBaseKey *SOURCE();

  /**
  Return the total number of cells available.
  */
  virtual vtkIdType GetNumberOfCells()=0;

  /**
  Return the number of points required for the named
  cell. For homogeneous datasets its always the same.
  */
  virtual int GetNumberOfCellPoints(vtkIdType id)=0;

  /**
  Copy the points from a cell into the provided buffer,
  buffer is expected to be large enought to hold all of the
  points.
  */
  virtual void GetCellPoints(vtkIdType id, float *pts)=0;


public:
  vtkTypeRevisionMacro(vtkSQOnDemandSource, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);


protected:
  vtkOnDemandSource(){}
  virtual ~vtkOnDemanndSource(){}

};

#endif
