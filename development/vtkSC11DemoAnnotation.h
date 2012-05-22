/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
// .NAME vtkSC11DemoAnnotation
// .SECTION Description
// Special annotation for the esnet SC11 demo.

#ifndef __vtkSC11DemoAnnotation_h
#define __vtkSC11DemoAnnotation_h

#include "vtkTableAlgorithm.h"

#include <string>
using std::string;
#include <vector>
using std::vector;

#ifdef SQTK_WITHOUT_MPI
typedef void * MPI_Comm;
#else
#include <mpi.h>
#endif

class VTK_EXPORT vtkSC11DemoAnnotation : public vtkTableAlgorithm
{
public:
  static vtkSC11DemoAnnotation* New();
  vtkTypeMacro(vtkSC11DemoAnnotation, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Get/Set the bandwidth data file
  vtkSetStringMacro(BWDataFile);
  vtkGetStringMacro(BWDataFile);

  // Description
  // Get/Set the time step file
  vtkSetStringMacro(TimeStepFile);
  vtkGetStringMacro(TimeStepFile);

  // Description
  // Get/Set the time data file
  void SetTimeDataFile(const char *filename);
  vtkGetStringMacro(TimeDataFile);

  // Description
  // not used
  vtkSetStringMacro(DummyFileListDomain);
  vtkGetStringMacro(DummyFileListDomain);
// BTX
protected:
  vtkSC11DemoAnnotation();
  ~vtkSC11DemoAnnotation();

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  virtual int RequestData(
        vtkInformation* request,
        vtkInformationVector** inputVector,
        vtkInformationVector* outputVector);

private:
  int WorldRank;
  int WorldSize;

  //BTX
  vector<string> Hosts;
  MPI_Comm BWComm;
  int InBWComm;
  //ETX

  char *DummyFileListDomain;
  char *BWDataFile;
  char *TimeStepFile;
  char *TimeDataFile;
  //BTX
  vector<string> TimeData;
  //ETX

private:
  vtkSC11DemoAnnotation(const vtkSC11DemoAnnotation&); // Not implemented
  void operator=(const vtkSC11DemoAnnotation&); // Not implemented
//ETX
};

#endif
