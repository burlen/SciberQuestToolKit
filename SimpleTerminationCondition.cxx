/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#include "SimpleTerminationCondition.h"


//-----------------------------------------------------------------------------
void SimpleTerminationCondition::InitializeColorMapper()
{
  // Initialize the mapper, color scheme as follows:
  // 0   -> problem domain
  // 1   -> s1
  //    ...
  // n   -> sn
  vector<string> names;
  names.push_back("noise");
  names.insert(names.end(),this->SurfaceNames.begin(),this->SurfaceNames.end());

  size_t nSurf=this->Surfaces.size(); // not adding any since cmap obj uses n+1.
  this->CMap.BuildColorMap(nSurf,names);
}
