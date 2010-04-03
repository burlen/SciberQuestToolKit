/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "BOVVectorImage.h"

//-----------------------------------------------------------------------------
BOVVectorImage::BOVVectorImage(
      MPI_Comm &comm,
      const char *xFileName,
      const char *yFileName,
      const char *zFileName,
      const char *name)
{
  this->X=new BOVScalarImage(comm,xFileName,name);
  this->Y=new BOVScalarImage(comm,yFileName);
  this->Z=new BOVScalarImage(comm,zFileName);
}

//-----------------------------------------------------------------------------
BOVVectorImage::~BOVVectorImage()
{
  delete this->X;
  delete this->Y;
  delete this->Z;
}

//-----------------------------------------------------------------------------
ostream &operator<<(ostream &os, const BOVVectorImage &vi)
{
  os << *vi.X << endl;
  os << *vi.Y << endl;
  os << *vi.Z << endl;
  return os;
}
