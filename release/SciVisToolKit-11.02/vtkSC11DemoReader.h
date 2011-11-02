/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
// .NAME vtkSC11DemoReader -- 
// .SECTION Description
//
// Implements the VTK style pipeline and manipulates and instance of
// BOVReader so that "brick of values" datasets, including time series,
// can be read in parallel.
//
// .SECTION See Also
// 

#ifndef __vtkSC11DemoReader_h
#define __vtkSC11DemoReader_h

#include "vtkDataSetAlgorithm.h"

// define this for cerr status.
// #define vtkSC11DemoReaderDEBUG

//BTX
class SC11DemoMetaData;
class SC11DemoReader;
class vtkInformationStringKey;
class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationIntegerVectorKey;
//ETX

class VTK_EXPORT vtkSC11DemoReader : public vtkDataSetAlgorithm
{
public:
  static vtkSC11DemoReader *New();
  vtkTypeRevisionMacro(vtkSC11DemoReader,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the file to read. Setting the file name opens
  // the file. Perhaps it's bad style but this is where open
  // fits best in VTK/PV pipeline execution.
  void SetFileName(const char *file);
  vtkGetStringMacro(FileName);

  // Description
  // Determine if the file can be read by opening it. If the open
  // succeeds then we assume th file is readable. Open is restricted
  // to the calling rank. Only one rank should call CanReadFile.
  int CanReadFile(const char *file);

  // Description:
  // Time domain discovery interface.
  int GetNumberOfTimeSteps();
  void GetTimeSteps(double *times);

protected:
  /// Pipeline internals.
  int RequestDataObject(vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  int RequestData(vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  int RequestInformation(vtkInformation*,vtkInformationVector**,vtkInformationVector*);

  vtkSC11DemoReader();
  ~vtkSC11DemoReader();

private:
  vtkSC11DemoReader(const vtkSC11DemoReader &); // Not implemented
  void operator=(const vtkSC11DemoReader &); // Not implemented

private:
  char *FileName;
  SC11DemoMetaData *MetaData;
  SC11DemoReader *Reader;
};

#endif

