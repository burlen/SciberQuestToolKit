/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#include "ClosedTerminationCondition.h"

//-----------------------------------------------------------------------------
void ClosedTerminationCondition::InitializeColorMapper()
{
  // Initialize the mapper, color scheme as follows:
  // 0   -> problem domain
  // 1   -> s1
  //    ...
  // n   -> sn
  // n+1 -> field null
  // n+2 -> short integration
  vector<string> names;
  names.push_back("domain bounds");
  names.insert(names.end(),this->SurfaceNames.begin(),this->SurfaceNames.end());
  names.push_back("feild null");
  names.push_back("short integration");

  size_t nSurf=this->Surfaces.size()+2; // only 2 bc problem domain is automatically included.
  this->CMap.BuildColorMap(nSurf,names);
}
