/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __OSXSystemInterface_h
#define __OSXSystemInterface_h

#include "UnixSystemInterface.h"

#ifdef __APPLE__

/// Mac interface.
class OSXSystemInterface : public UnixSystemInterface
{
public:
  OSXSystemInterface() : MemoryTotal(0) {}
  virtual ~OSXSystemInterface(){}

  /**
  Return the total amount of physical RAM avaiable on the system.
  */
  virtual unsigned long long GetMemoryTotal(){ return this->MemoryTotal; }

  /**
  Return the amount of physical RAM used by this process.
  */
  virtual unsigned long long GetMemoryUsed(){ return 0; }


private:
  unsigned long long MemoryTotal;
};

#else
  typedef SystemInterface OSXSystemInterface;
#endif

#endif
