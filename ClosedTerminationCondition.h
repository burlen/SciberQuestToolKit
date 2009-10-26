/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef ClosedTerminationCondition_h
#define ClosedTerminationCondition_h

#include "TerminationCondition.h"

// The closed termination condition asigns a unique color
// to all topological classes that arise, including termination
// due to the special cases of exiting the problem domain,
// short integration, and stagnation.
//
class ClosedTerminationCondition : public TerminationCondition
{
public:
  // Description:
  // Build the color mapper with the folowing scheme:
  //
  // 0   -> field null
  // 1   -> s1
  //    ...
  // n   -> sn
  // n+1 -> problem domain
  // n+2 -> short integration
  virtual void InitializeColorMapper();

  // Description:
  // Return the indentifier for the special termination cases.
  virtual int GetProblemDomainSurfaceId(){ return 0; }
  virtual int GetFieldNullId(){ return this->Surfaces.size()+1; }
  virtual int GetShortIntegrationId(){ return this->Surfaces.size()+2; }
};

#endif
