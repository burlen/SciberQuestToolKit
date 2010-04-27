/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "BOVMetaData.h"

#include "Tuple.hxx"
#include "PrintUtils.h"

//-----------------------------------------------------------------------------
void BOVMetaData::Pack(Stream &os)
{
  os.Pack(this->IsOpen);
  os.Pack(this->PathToBricks);
  os.Pack(this->Domain);
  os.Pack(this->Decomp);
  os.Pack(this->Subset);
  os.Pack(this->Arrays);
  os.Pack(this->TimeSteps);
}

//-----------------------------------------------------------------------------
void BOVMetaData::UnPack(Stream &is)
{
  is.UnPack(this->IsOpen);
  is.UnPack(this->PathToBricks);
  is.UnPack(this->Domain);
  is.UnPack(this->Decomp);
  is.UnPack(this->Subset);
  is.UnPack(this->Arrays);
  is.UnPack(this->TimeSteps);
}

//-----------------------------------------------------------------------------
void BOVMetaData::Print(ostream &os) const
{
  os << "BOVMetaData: " << this << endl;
  os << "\tIsOpen: " << this->IsOpen << endl;
  os << "\tPathToBricks: " << this->PathToBricks << endl;
  os << "\tDomain: "; this->Domain.Print(os) << endl;
  os << "\tSubset: "; this->Subset.Print(os) << endl;
  os << "\tDecomp: "; this->Decomp.Print(os) << endl;
  os << "\tArrays: " << this->Arrays << endl;
  os << "\tTimeSteps: " << this->TimeSteps << endl;
}
