/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __LinuxSystemInterface_h
#define __LinuxSystemInterface_h

#include "UnixSystemInterface.h"

#ifdef __linux__

/// Linux interface.
class LinuxSystemInterface : public UnixSystemInterface
{
public:
  LinuxSystemInterface();
  virtual ~LinuxSystemInterface(){}

  /**
  Return the total amount of physical RAM avaiable on the system.
  */
  virtual unsigned long long GetMemoryTotal(){ return this->MemoryTotal; }

  /**
  Return the amount of physical RAM used by this process.
  */
  virtual unsigned long long GetMemoryUsed(){ return this->GetVmRSS(); }

  /**
  More detailed information specific to linux.
  */
  unsigned long long GetVmRSS();
  unsigned long long GetVmPeak();
  unsigned long long GetVmSize();
  unsigned long long GetVmLock();
  unsigned long long GetVmHWM();
  unsigned long long GetVmData();
  unsigned long long GetVmStack();
  unsigned long long GetVmExec();
  unsigned long long GetVmLib();
  unsigned long long GetVmPTE();
  unsigned long long GetVmSwap();

private:
  unsigned long long GetStatusField(const char *name);

private:
  unsigned long long MemoryTotal;
};

#else
  typedef SystemInterface LinuxSystemInterface;
#endif

#endif
