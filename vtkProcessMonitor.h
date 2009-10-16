/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkProcessMonitor.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProcessMonitor - create an array of quadrilaterals located in a plane
// .SECTION Description

#ifndef __vtkProcessMonitor_h
#define __vtkProcessMonitor_h

#include "vtkPolyDataAlgorithm.h"

class vtkMultiProcessController;

#include<string>
using std::string;

#include<iostream>
using std::cerr;
using std::endl;

// define the following ot enable debuging io
// #define vtkProcessMonitorDEBUG

class VTK_GRAPHICS_EXPORT vtkProcessMonitor : public vtkPolyDataAlgorithm
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkProcessMonitor,vtkPolyDataAlgorithm);

  // Description:
  // Construct plane perpendicular to z-axis, resolution 1x1, width
  // and height 1.0, and centered at the origin.
  static vtkProcessMonitor *New();

  // Description:
  // Get the configuration in a stream.
  // nProcs proc1_hostname proc1_pid ... procN_hostname procN_pid
  vtkGetStringMacro(ConfigStream);
  vtkSetStringMacro(ConfigStream);

protected:
  vtkProcessMonitor();
  virtual ~vtkProcessMonitor();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *,vtkInformationVector **,vtkInformationVector *);

private:
  vtkMultiProcessController *Controller;
  int ProcId;
  int NProcs;
  int Pid;
  string Hostname;
  char *ConfigStream;

private:
  vtkProcessMonitor(const vtkProcessMonitor&);  // Not implemented.
  void operator=(const vtkProcessMonitor&);  // Not implemented.
};

#endif
