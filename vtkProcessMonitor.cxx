/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkProcessMonitor.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProcessMonitor.h"



#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDataObject.h"
#include "vtkPolyData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMetaDataKeys.h"
#include "vtkMultiProcessController.h"

#include<string>
using std::string;

#include<iostream>
using std::cerr;
using std::endl;

#include<sstream>
using std::ostringstream;

#include <unistd.h>

#include "mpi.h"

vtkCxxRevisionMacro(vtkProcessMonitor, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkProcessMonitor);

//----------------------------------------------------------------------------
vtkProcessMonitor::vtkProcessMonitor()
    :
  Controller(0),
  ProcId(0),
  NProcs(1),
  Pid(0),
  Hostname("localhost"),
  ConfigStream(0)
{
  #if defined vtkProcessMonitorDEBUG
    cerr << "====================================================================vtkProcessMonitor" << endl;
  #endif
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkProcessMonitor::~vtkProcessMonitor()
{
  #if defined vtkProcessMonitorDEBUG
    cerr << "====================================================================~vtkProcessMonitor" << endl;
  #endif
  this->SetConfigStream(0);
}

//----------------------------------------------------------------------------
int vtkProcessMonitor::RequestInformation(
  vtkInformation *request,
  vtkInformationVector **inInfos,
  vtkInformationVector *outInfos)
{
  #if defined vtkProcessMonitorDEBUG
    cerr << "====================================================================RequestInformation" << endl;
  #endif

  this->vtkPolyDataAlgorithm::RequestInformation(request,inInfos,outInfos);

  if (this->Controller)
    {
    return 1;
    }

  this->Controller=vtkMultiProcessController::GetGlobalController();
  this->ProcId=this->Controller->GetLocalProcessId();
  this->NProcs=this->Controller->GetNumberOfProcesses();

  this->Pid=getpid();

  char hostname[1024];
  gethostname(hostname,1024);
  this->Hostname=hostname;
  int hnLen=strlen(hostname)+1;

  cerr << this->ProcId << " " << hnLen << " " << hostname << " " << this->Pid << endl;

  // set root up for gather pid and hostname sizes
  int *pids=0;
  int *hnLens=0;
  int *hnDispls=0;
  if (this->ProcId==0)
    {
    pids=(int *)malloc(this->NProcs*sizeof(int));
    hnLens=(int *)malloc(this->NProcs*sizeof(int));
    hnDispls=(int *)malloc(this->NProcs*sizeof(int));
    }

  // gather
  int stat=0;
  stat|=MPI_Gather(&hnLen,1,MPI_INT,hnLens,1,MPI_INT,0,MPI_COMM_WORLD);
  stat|=MPI_Gather(&this->Pid,1,MPI_INT,pids,1,MPI_INT,0,MPI_COMM_WORLD);

  // set root up for gather hostnames
  char *hostnames=0;
  if (this->ProcId==0)
    {
    int n=0;
    for (int i=0; i<this->NProcs; ++i)
      {
      hnDispls[i]=n;
      n+=hnLens[i];
      }
    hostnames=(char *)malloc(n*sizeof(char));
    }

  // gather
  stat|=MPI_Gatherv(hostname,hnLen,MPI_CHAR,hostnames,hnLens,hnDispls,MPI_CHAR,0,MPI_COMM_WORLD);

  // check for comm errors
  if (stat!=MPI_SUCCESS)
    {
    vtkWarningMacro("MPI communication error. Could not initialize object. Aborting.");
    return 0;
    }

  // root saves the configuration to an ascii stream where it
  // will be accessed by pv client.
  if (this->ProcId==0)
    {
    ostringstream os;
    os << this->NProcs;

    int *pPid=pids;
    char *pHn=hostnames;
    for (int i=0; i<this->NProcs; ++i)
      {
      os << " " <<pHn;
      pHn+=hnLens[i];
      os << " " << *pPid;
      ++pPid;
      }

    this->SetConfigStream(os.str().c_str());
    cerr << os.str() << endl;

    // root cleans up.
    free(pids);
    free(hnLens);
    free(hnDispls);
    free(hostnames);
    }

  // this->Modified();

  return 1;
}


//----------------------------------------------------------------------------
int vtkProcessMonitor::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outInfos)
{
  #if defined vtkProcessMonitorDEBUG
    cerr << "====================================================================RequestData" << endl;
  #endif

  vtkInformation *outInfo=outInfos->GetInformationObject(0);
  vtkPolyData *pdOut=dynamic_cast<vtkPolyData*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (pdOut==0)
    {
    vtkWarningMacro("No output data found. Aborting request.");
    return 1;
    }
  pdOut->Initialize();

  return 1;
}

//----------------------------------------------------------------------------
void vtkProcessMonitor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  // TODO
}
