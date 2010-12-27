/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#include "vtkSQProcessMonitor.h"

#define vtkSQProcessMonitorDEBUG

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDataObject.h"
#include "vtkPolyData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include <mpi.h>


#include<string>
using std::string;
#include<iostream>
using std::cerr;
using std::endl;
#include<sstream>
using std::ostringstream;
using std::istringstream;

#include "SQMacros.h"
#include "FsUtils.h"
#include "postream.h"
#include "SystemType.h"
#include "SystemInterface.h"
#include "SystemInterfaceFactory.h"


vtkCxxRevisionMacro(vtkSQProcessMonitor, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkSQProcessMonitor);

//-----------------------------------------------------------------------------
vtkSQProcessMonitor::vtkSQProcessMonitor()
    :
  WorldRank(0),
  WorldSize(1),
  Pid(0),
  HostName("localhost"),
  ConfigStream(0),
  MemoryUseStream(0),
  InformationMTime(0),
  ServerSystem(0)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::vtkSQProcessMonitor" << endl;
  #endif

  // standard pv info
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->ServerSystem=SystemInterfaceFactory::NewSystemInterface();

  // gather information about this run.
  this->Pid=this->ServerSystem->GetProcessId();
  this->HostName=this->ServerSystem->GetHostName();
  char *hostname=const_cast<char*>(this->HostName.c_str());
  int hnLen=this->HostName.size()+1;

  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (mpiOk)
    {
    MPI_Comm_size(MPI_COMM_WORLD,&this->WorldSize);
    MPI_Comm_rank(MPI_COMM_WORLD,&this->WorldRank);

    // set root up for gather pid and hostname sizes
    int *pids=0;
    int *hnLens=0;
    int *hnDispls=0;
    unsigned long long *caps=0;
    if (this->WorldRank==0)
      {
      pids=(int *)malloc(this->WorldSize*sizeof(int));
      caps=(unsigned long long *)malloc(this->WorldSize*sizeof(unsigned long long));
      hnLens=(int *)malloc(this->WorldSize*sizeof(int));
      hnDispls=(int *)malloc(this->WorldSize*sizeof(int));
      }

    // gather
    MPI_Gather(&hnLen,1,MPI_INT,hnLens,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Gather(&this->Pid,1,MPI_INT,pids,1,MPI_INT,0,MPI_COMM_WORLD);
    unsigned long long cap=this->ServerSystem->GetMemoryTotal();
    MPI_Gather(&cap,1,MPI_UNSIGNED_LONG_LONG,caps,1,MPI_UNSIGNED_LONG_LONG,0,MPI_COMM_WORLD);

    // set root up for gather hostnames
    char *hostnames=0;
    if (this->WorldRank==0)
      {
      int n=0;
      for (int i=0; i<this->WorldSize; ++i)
        {
        hnDispls[i]=n;
        n+=hnLens[i];
        }
      hostnames=(char *)malloc(n*sizeof(char));
      }

    // gather
    MPI_Gatherv(
          hostname,
          hnLen,
          MPI_CHAR,
          hostnames,
          hnLens,
          hnDispls,
          MPI_CHAR,
          0,
          MPI_COMM_WORLD);

    // root saves the configuration to an ascii stream where it
    // will be accessed by pv client.
    if (this->WorldRank==0)
      {
      ostringstream os;
      os
        << SYSTEM_TYPE << " "
        << this->WorldSize;

      int *pPid=pids;
      char *pHn=hostnames;
      unsigned long long *pCap=caps;
      for (int i=0; i<this->WorldSize; ++i)
        {
        os
          << " " << pHn
          << " " << *pPid
          << " " << *pCap;

        pHn+=hnLens[i];
        ++pPid;
        ++pCap;
        }

      this->SetConfigStream(os.str().c_str());
      /// cerr << os.str() << endl;

      // root cleans up.
      free(pids);
      free(caps);
      free(hnLens);
      free(hnDispls);
      free(hostnames);
      }
    }
  else
    {
    ostringstream os;
    os
      << SYSTEM_TYPE << " "
      << 1 << " "
      << this->HostName << " "
      << this->Pid << " "
      << this->ServerSystem->GetMemoryTotal();
    this->SetConfigStream(os.str().c_str());
    }
}

//-----------------------------------------------------------------------------
vtkSQProcessMonitor::~vtkSQProcessMonitor()
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::~vtkSQProcessMonitor" << endl;
  #endif
  this->SetConfigStream(0);
  this->SetMemoryUseStream(0);

  delete this->ServerSystem;
}

//-----------------------------------------------------------------------------
void vtkSQProcessMonitor::SetEnableBacktraceHandler(int enable)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::SetEnableBacktraceHandler" << endl;
  #endif

  this->ServerSystem->StackTraceOnError(enable);
}

//-----------------------------------------------------------------------------
void vtkSQProcessMonitor::SetEnableFE_ALL(int enable)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::SetEnableFE_ALL" << endl;
  #endif

  this->ServerSystem->CatchAllFloatingPointExceptions(enable);
}


//-----------------------------------------------------------------------------
void vtkSQProcessMonitor::SetEnableFE_DIVBYZERO(int enable)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::SetEnableFE_DIVBYZERO" << endl;
  #endif

  this->ServerSystem->CatchDIVBYZERO(enable);
}

//-----------------------------------------------------------------------------
void vtkSQProcessMonitor::SetEnableFE_INEXACT(int enable)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::SetEnableFE_INEXACT" << endl;
  #endif

  this->ServerSystem->CatchINEXACT(enable);
}

//-----------------------------------------------------------------------------
void vtkSQProcessMonitor::SetEnableFE_INVALID(int enable)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::SetEnableFE_INVALID" << endl;
  #endif

  this->ServerSystem->CatchINVALID(enable);
}

//-----------------------------------------------------------------------------
void vtkSQProcessMonitor::SetEnableFE_OVERFLOW(int enable)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::SetEnableFE_OVERFLOW" << endl;
  #endif

  this->ServerSystem->CatchOVERFLOW(enable);
}

//-----------------------------------------------------------------------------
void vtkSQProcessMonitor::SetEnableFE_UNDERFLOW(int enable)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::SetEnableFE_UNDERFLOW" << endl;
  #endif

  this->ServerSystem->CatchUNDERFLOW(enable);
}

//-----------------------------------------------------------------------------
int vtkSQProcessMonitor::RequestInformation(
      vtkInformation *request,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::RequestInformation" << endl;
  #endif

  this->vtkPolyDataAlgorithm::RequestInformation(request,inInfos,outInfos);

  // get the local memory use
  unsigned long long localMemoryUse=this->ServerSystem->GetMemoryUsed();
  cerr << localMemoryUse << endl;

  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (mpiOk)
    {
    // set root up for gather memory usage
    unsigned long long  *remoteMemoryUse=0;
    if (this->WorldRank==0)
      {
      remoteMemoryUse=(unsigned long long *)malloc(this->WorldSize*sizeof(unsigned long long));
      }

    // gather
    MPI_Gather(
          &localMemoryUse,
          1,
          MPI_UNSIGNED_LONG_LONG,
          remoteMemoryUse,
          1,
          MPI_UNSIGNED_LONG_LONG,
          0,
          MPI_COMM_WORLD);

    // root saves the configuration to an ascii stream where it
    // will be accessed by pv client.
    if (this->WorldRank==0)
      {
      ostringstream os;
      os << this->WorldSize;

      for (int i=0; i<this->WorldSize; ++i)
        {
        os << " " << remoteMemoryUse[i];
        }

      this->SetMemoryUseStream(os.str().c_str());
      cerr << os.str() << endl;

      // root cleans up.
      free(remoteMemoryUse);
      }
    }
  else
    {
    ostringstream os;
    os << 1 << " " << localMemoryUse;
    this->SetMemoryUseStream(os.str().c_str());
    }

  ++this->InformationMTime;
  cerr << this->InformationMTime;

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQProcessMonitor::RequestData(
      vtkInformation *vtkNotUsed(request),
      vtkInformationVector **vtkNotUsed(inputVector),
      vtkInformationVector *outInfos)
{
  #if defined vtkSQProcessMonitorDEBUG
    cerr << "===============================vtkSQProcessMonitor::RequestData" << endl;
  #endif

  vtkInformation *outInfo=outInfos->GetInformationObject(0);
  vtkPolyData *pdOut=dynamic_cast<vtkPolyData*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (pdOut==0)
    {
    vtkErrorMacro("Empty output.");
    return 1;
    }
  pdOut->Initialize();

  return 1;
}

//----------------------------------------------------------------------------
void vtkSQProcessMonitor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  // TODO
}
