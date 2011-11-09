/*=========================================================================

  Program:   ParaView
  Module:    vtkSC11DemoAnnotation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include <mpi.h>

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
  // Get/Set the time data file
  vtkSetStringMacro(TimeDataFile);
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
  char *TimeDataFile;

private:
  vtkSC11DemoAnnotation(const vtkSC11DemoAnnotation&); // Not implemented
  void operator=(const vtkSC11DemoAnnotation&); // Not implemented
//ETX
};

#endif

