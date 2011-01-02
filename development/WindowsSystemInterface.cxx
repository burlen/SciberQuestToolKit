/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#include "WindowsSystemInterface.h"

#ifdef _WIN32

#include <iostream>
using std::cerr;
using std::endl;

//#include "Winsock2.h"

//-----------------------------------------------------------------------------
WindowsSystemInterface::WindowsSystemInterface()
{
  this->Pid=GetCurrentProcessId();

  MEMORYSTATUSEX statex;
  statex.dwLength=sizeof(statex);
  GlobalMemoryStatusEx(&statex);
  this->MemoryTotal=statex.ullTotalPhys/1024;

  this->HProc=OpenProcess(
      PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,
      false,
      this->Pid);
  if (this->HProc==0)
    {
    cerr
      << "Error: failed to obtain process handle for "
      << this->Pid << "."
      << endl;
    }

  char hostName[1024];
  int iErr=gethostname(hostName,1024);
  if (iErr)
    {
    cerr << "Error: failed to obtain the hostname." << endl;
    this->HostName="localhost";
    }
  else
    {
    this->HostName=hostName;
    }
}

//-----------------------------------------------------------------------------
WindowsSystemInterface::~WindowsSystemInterface()
{
  CloseHandle(this->HProc);
}

//-----------------------------------------------------------------------------
unsigned long long WindowsSystemInterface::GetMemoryUsed()
{
  PROCESS_MEMORY_COUNTERS pmc;
  int ok=GetProcessMemoryInfo(this->HProc,&pmc,sizeof(pmc));
  if (!ok)
    {
    cerr << "Failed to obtain memory information." << endl;
    return -1;
    }
  return pmc.WorkingSetSize/1024;
}

#endif
