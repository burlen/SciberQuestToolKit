/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __BOVScalarImage_h
#define __BOVScalarImage_h

#ifdef SQTK_WITHOUT_MPI
typedef void * MPI_Comm;
typedef void * MPI_Info;
typedef void * MPI_File;
#else
#include <mpi.h>
#endif

#include <string>
using std::string;
#include <iostream>
using std::ostream;

/// Handle to file containing a scalar array.
class BOVScalarImage
{
public:
  BOVScalarImage(
      MPI_Comm comm,
      MPI_Info hints,
      const char *fileName,
      int mode);

  BOVScalarImage(
      MPI_Comm comm,
      MPI_Info hints,
      const char *fileName,
      const char *name,
      int mode);

  ~BOVScalarImage();

  MPI_File GetFile() const { return this->File; }
  const char *GetName() const { return this->Name.c_str(); }
  const char *GetFileName() const { return this->FileName.c_str(); }

private:
  MPI_File File;
  string FileName;
  string Name;

private:
  const BOVScalarImage &operator=(const BOVScalarImage &other);
  BOVScalarImage(const BOVScalarImage &other);
  BOVScalarImage();
};

ostream &operator<<(ostream &os, const BOVScalarImage &si);

#endif
