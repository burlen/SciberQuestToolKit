/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "BOVScalarImage.h"

//-----------------------------------------------------------------------------
MPI_File Open(MPI_Comm comm, const char *fileName)
{
  MPI_File file=0;
  int iErr;
  const int eStrLen=2048;
  char eStr[eStrLen]={'\0'};
  // Open the file
  iErr=MPI_File_open(
      comm,
      const_cast<char *>(fileName),
      MPI_MODE_RDONLY,
      MPI_INFO_NULL,
      &file);
  if (iErr!=MPI_SUCCESS)
    {
    MPI_Error_string(iErr,eStr,const_cast<int *>(&eStrLen));
    cerr << "Error opeing file: " << fileName << endl;
    cerr << eStr << endl;
    file=0;
    }
  return file;
}

//-----------------------------------------------------------------------------
BOVScalarImage::BOVScalarImage(
    MPI_Comm &comm,
    MPI_Info hints,
    const char *fileName)
{
  this->File=Open(comm,fileName);
  this->FileName=fileName;
}

//-----------------------------------------------------------------------------
BOVScalarImage::BOVScalarImage(MPI_Comm comm, const char *fileName, const char *name)
{
  this->File=Open(comm,fileName);
  this->FileName=fileName;
  this->Name=name;
}

//-----------------------------------------------------------------------------
BOVScalarImage::~BOVScalarImage()
{
  if (this->File)
    {
    MPI_File_close(&this->File);
    }
}

//-----------------------------------------------------------------------------
ostream &operator<<(ostream &os, const BOVScalarImage &si)
{
  os << si.GetFileName() << " " << si.GetName() << " " << si.GetFile();
  return os;
}
