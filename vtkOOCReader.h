/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef vtkOOCReader_h
#define vtkOOCReader_h

#include "vtkObject.h"

class vtkInformation;
class vtkInformationObjectBaseKey;
class vtkInformationDoubleVectorKey;
class vtkDataSet;

/// Interface class for Out-Of-Core (OOC) file access.
/**
Allow one to read in chuncks of data as needed. A specific
chunk of data is identified to be read by providing a point
in which the chunk should reside. The implementation may
implement caching as desired but this is not strictly 
neccessary. This class also provides a number of keys
that should be used by meta readers.
*/
class VTK_EXPORT vtkOOCReader : public vtkObject
{
public:
  /// \section Pipeline \@{
  /**
  This key is used to pass the actual reader from the meta reader
  to downstream filters. The meta reader at the head of the pipeline
  initializes and sets the reader into the pipeline information
  then down strem filters may read data as needed. The object set 
  must implement the vtkOOCReader interface and be reader to read
  once set into the pipeline.
  */
  static vtkInformationObjectBaseKey *READER();
  /**
  This key is used to pass the dataset bounds downstream.
  */
  static vtkInformationDoubleVectorKey *BOUNDS();

public:
  vtkTypeRevisionMacro(vtkOOCReader, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// \section IO \@{
  /**
  Read the data in a neighborhood bounded by the box.
  */
  virtual vtkDataSet *Read(double b[6])=0;

  /**
  Read the data in a neighborhood centered about a point, p.
  The number of cells read is implementation specific but is
  controlled by the dimension parameter.
  */
  virtual vtkDataSet *ReadNeighborhood(double p[3], int size)=0;

  /**
  Turn on an array to be read.
  */
  virtual void ActivateArray(const char *arrayName)=0;
  /**
  Turn off an array to be read.
  */
  virtual void DeActivateArray(const char *arrayName)=0;
  virtual void DeActivateAllArrays()=0;

  /**
  Set the time step index or time to read.
  */
  vtkSetMacro(Time,double);
  vtkGetMacro(Time,double);
  vtkSetMacro(TimeIndex,int);
  vtkGetMacro(TimeIndex,int);
  /// \@}

protected:
  vtkOOCReader()
      :
    TimeIndex(0),
    Time(0)
      {};
  virtual ~vtkOOCReader(){};

private:
  vtkOOCReader(const vtkOOCReader &o);
  const vtkOOCReader &operator=(const vtkOOCReader &o);

protected:
  int TimeIndex;
  double Time;
};

#endif
