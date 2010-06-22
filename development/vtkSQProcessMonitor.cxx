/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#include "vtkSQProcessMonitor.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDataObject.h"
#include "vtkPolyData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkSQMetaDataKeys.h"
#include "vtkMultiProcessController.h"

#include <mpi.h>
#include <unistd.h>
#include <fenv.h>
#include <signal.h>
#include <execinfo.h>

#include<string>
using std::string;
#include<iostream>
using std::cerr;
using std::endl;
#include<sstream>
using std::ostringstream;

// Pointer to some process monitor instance provides
// access to contextual information during signal hander.
static const vtkSQProcessMonitor *_ProcMonInstance=0;

//*****************************************************************************
void backtrace_handler(
      int sigNo,
      siginfo_t *sigInfo,
      void */*sigContext*/)
{
  cerr << "[" << _ProcMonInstance->WorldRank << "] ";

  switch (sigNo)
    {
    case SIGFPE:
      cerr << "Caught SIGFPE type ";
      switch (sigInfo->si_code)
        {
        case FPE_INTDIV:
          cerr << "integer division by zero";
          break;

        case FPE_INTOVF:
          cerr << "integer overflow";
          break;

        case FPE_FLTDIV:
          cerr << "floating point divide by zero";
          break;

        case FPE_FLTOVF:
          cerr << "floating point overflow";
          break;

        case FPE_FLTUND:
          cerr << "floating point underflow";
          break;

        case FPE_FLTRES:
          cerr << "floating point inexact result";
          break;

        case FPE_FLTINV:
          cerr << "floating point invalid operation";
          break;

        case FPE_FLTSUB:
          cerr << "floating point subscript out of range";
          break;

        default:
          cerr << "unknown type";
          break;
        }
      break;

    case SIGSEGV:
      cerr << "Caught SIGSEGV type ";
      switch (sigInfo->si_code)
        {
        case SEGV_MAPERR:
          cerr << "address not mapped to object";
          break;

        case SEGV_ACCERR:
          cerr << "invalid permission for mapped object";
          break;

        default:
          cerr << "unknown type";
          break;
        }
      break;

    case SIGBUS:
      cerr << "Caught SIGBUS type ";
      switch (sigInfo->si_code)
        {
        case BUS_ADRALN:
          cerr << "invalid address alignment";
          break;

        case BUS_ADRERR:
          cerr << "non-exestent physical address";
          break;

        case BUS_OBJERR:
          cerr << "object specific hardware error";
          break;

        default:
          cerr << "unknown type";
          break;
        }
      break;

    case SIGILL:
      cerr << "Caught SIGILL type ";
      switch (sigInfo->si_code)
        {
        case ILL_ILLOPC:
          cerr << "illegal opcode";
          break;

        case ILL_ILLOPN:
          cerr << "illegal operand";
          break;

        case ILL_ILLADR:
          cerr << "illegal addressing mode.";
          break;

        case ILL_ILLTRP:
          cerr << "illegal trap";

        case ILL_PRVOPC:
          cerr << "privileged opcode";
          break;

        case ILL_PRVREG:
          cerr << "privileged register";
          break;

        case ILL_COPROC:
          cerr << "co-processor error";
          break;

        case ILL_BADSTK:
          cerr << "internal stack error";
          break;

        default:
          cerr << "unknown type";
          break;
        }
      break;

    default:
      cerr << "Caught " << sigNo;
      break;
    }

  // dump a backtrace to stderr
  cerr << "." << endl;

  void *stack[128];
  int n=backtrace(stack,128);
  backtrace_symbols_fd(stack,n,2);

  abort();
}


vtkCxxRevisionMacro(vtkSQProcessMonitor, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkSQProcessMonitor);

//-----------------------------------------------------------------------------
vtkSQProcessMonitor::vtkSQProcessMonitor()
    :
  WorldRank(0),
  WorldSize(1),
  Pid(0),
  Hostname("localhost"),
  ConfigStream(0)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::vtkSQProcessMonitor" << endl;
  #endif

  if (_ProcMonInstance==0)
    {
    _ProcMonInstance=this;
    }

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkSQProcessMonitor::~vtkSQProcessMonitor()
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::~vtkSQProcessMonitor" << endl;
  #endif
  this->SetConfigStream(0);
}

//-----------------------------------------------------------------------------
void vtkSQProcessMonitor::SetEnableBacktraceHandler(int enable)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::SetEnableBacktraceHandler" << endl;
  #endif

  static int saOrigValid=0;
  static struct sigaction saSEGVOrig;
  static struct sigaction saILLOrig;
  static struct sigaction saBUSOrig;
  static struct sigaction saFPEOrig;

  if (enable)
    {
    // save the current actions
    sigaction(SIGSEGV,0,&saSEGVOrig);
    sigaction(SIGILL,0,&saILLOrig);
    sigaction(SIGBUS,0,&saBUSOrig);
    sigaction(SIGFPE,0,&saFPEOrig);

    saOrigValid=1;

    // install ours
    struct sigaction sa;
    sa.sa_sigaction=&backtrace_handler;
    sa.sa_flags=SA_SIGINFO|SA_RESTART;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGSEGV,&sa,0);
    sigaction(SIGILL,&sa,0);
    sigaction(SIGBUS,&sa,0);
    sigaction(SIGFPE,&sa,0);
    }
  else
    {
    if (saOrigValid)
      {
      // restore previous actions
      sigaction(SIGSEGV,&saSEGVOrig,0);
      sigaction(SIGILL,&saILLOrig,0);
      sigaction(SIGBUS,&saBUSOrig,0);
      sigaction(SIGFPE,&saFPEOrig,0);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSQProcessMonitor::SetEnableFE_ALL(int enable)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::SetEnableFE_ALL" << endl;
  #endif

  if (enable)
    {
    feenableexcept(FE_ALL_EXCEPT);
    }
  else
    {
    fedisableexcept(FE_ALL_EXCEPT);
    }
}


//-----------------------------------------------------------------------------
void vtkSQProcessMonitor::SetEnableFE_DIVBYZERO(int enable)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::SetEnableFE_DIVBYZERO" << endl;
  #endif

  if (enable)
    {
    feenableexcept(FE_DIVBYZERO);
    }
  else
    {
    fedisableexcept(FE_DIVBYZERO);
    }
}

//-----------------------------------------------------------------------------
void vtkSQProcessMonitor::SetEnableFE_INEXACT(int enable)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::SetEnableFE_INEXACT" << endl;
  #endif

  if (enable)
    {
    feenableexcept(FE_INEXACT);
    }
  else
    {
    fedisableexcept(FE_INEXACT);
    }
}

//-----------------------------------------------------------------------------
void vtkSQProcessMonitor::SetEnableFE_INVALID(int enable)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::SetEnableFE_INVALID" << endl;
  #endif

  if (enable)
    {
    feenableexcept(FE_INVALID);
    }
  else
    {
    fedisableexcept(FE_INVALID);
    }
}

//-----------------------------------------------------------------------------
void vtkSQProcessMonitor::SetEnableFE_OVERFLOW(int enable)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::SetEnableFE_OVERFLOW" << endl;
  #endif

  if (enable)
    {
    feenableexcept(FE_OVERFLOW);
    }
  else
    {
    fedisableexcept(FE_OVERFLOW);
    }
}

//-----------------------------------------------------------------------------
void vtkSQProcessMonitor::SetEnableFE_UNDERFLOW(int enable)
{
  #if defined vtkSQProcessMonitorDEBUG
  cerr << "===============================vtkSQProcessMonitor::SetEnableFE_UNDERFLOW" << endl;
  #endif

  if (enable)
    {
    feenableexcept(FE_UNDERFLOW);
    }
  else
    {
    fedisableexcept(FE_UNDERFLOW);
    }
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

  static int initialized=0;
  if (initialized)
    {
    return 1;
    }
  initialized=1;

  this->Pid=getpid();

  char hostname[1024];
  gethostname(hostname,1024);
  this->Hostname=hostname;
  int hnLen=strlen(hostname)+1;


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
    if (this->WorldRank==0)
      {
      pids=(int *)malloc(this->WorldSize*sizeof(int));
      hnLens=(int *)malloc(this->WorldSize*sizeof(int));
      hnDispls=(int *)malloc(this->WorldSize*sizeof(int));
      }

    // gather
    MPI_Gather(&hnLen,1,MPI_INT,hnLens,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Gather(&this->Pid,1,MPI_INT,pids,1,MPI_INT,0,MPI_COMM_WORLD);

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
      os << this->WorldSize;

      int *pPid=pids;
      char *pHn=hostnames;
      for (int i=0; i<this->WorldSize; ++i)
        {
        os << " " << pHn;
        pHn+=hnLens[i];
        os << " " << *pPid;
        ++pPid;
        }

      this->SetConfigStream(os.str().c_str());
      /// cerr << os.str() << endl;

      // root cleans up.
      free(pids);
      free(hnLens);
      free(hnDispls);
      free(hostnames);
      }
    }
  else
    {
    ostringstream os;
    os << 1 << " " << this->Hostname << " " << this->Pid;
    this->SetConfigStream(os.str().c_str());
    }

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
