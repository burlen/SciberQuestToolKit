/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#ifndef __WindowsSystemInterface_h
#define __WindowsSystemInterface_h

#include "SystemInterface.h"

#ifdef _WIN32

#include <string>
using std::string;

#include "windows.h"
#include "psapi.h"


/// Windows interface.
class WindowsSystemInterface : public SystemInterface
{
public:
  WindowsSystemInterface();
  virtual ~WindowsSystemInterface();

  /**
  Return the total amount of physical RAM avaiable on the system.
  */
  virtual unsigned long long GetMemoryTotal(){ return this->MemoryTotal; }

  /**
  Return the amount of physical RAM used by this process.
  */
  virtual unsigned long long GetMemoryUsed();

  /**
  Return the processs identifier of this process.
  */
  virtual int GetProcessId(){ return this->Pid; }

  /**
  Return the hostname.
  */
  virtual string GetHostName(){ return this->HostName; }

  /**
  Execute the given command in a new process.
  */
  virtual int Exec(string &cmd){ return -1; }

  /**
  If set will print a stack trace for segfault, gp fault, floating point
  exception etc...
  */
  virtual void StackTraceOnError(int ){}

  /**
  Turn on/off floating point exceptions.
  */
  virtual void CatchAllFloatingPointExceptions(int ){}
  virtual void CatchDIVBYZERO(int ){}
  virtual void CatchINEXACT(int ){}
  virtual void CatchINVALID(int ){}
  virtual void CatchOVERFLOW(int ){}
  virtual void CatchUNDERFLOW(int ){}

private:
  int Pid;
  HANDLE HProc;
  string HostName;
  unsigned long long MemoryTotal;
};

#else
  typedef SystemInterface WindowsSystemInterface;
#endif

#endif
