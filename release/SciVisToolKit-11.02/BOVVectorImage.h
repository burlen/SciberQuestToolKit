/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef BOVVectorImage_h
#define BOVVectorImage_h

#include "BOVScalarImage.h"

#include<iostream>
using std::ostream;
using std::endl;

/// Handle to a vector (3 scalar handles).
class BOVVectorImage
{
public:
  BOVVectorImage(
      MPI_Comm comm,
      MPI_Info hints,
      const char *xFileName,
      const char *yFileName,
      const char *zFileName,
      const char *name);
  ~BOVVectorImage();

  MPI_File GetXFile() const { return this->X->GetFile(); }
  MPI_File GetYFile() const { return this->Y->GetFile(); }
  MPI_File GetZFile() const { return this->Z->GetFile(); }
  const char *GetName() const { return this->X->GetName(); }

private:
  BOVScalarImage *X;
  BOVScalarImage *Y;
  BOVScalarImage *Z;

private:
  friend ostream &operator<<(ostream &os, const BOVVectorImage &si);
  const BOVVectorImage &operator=(const BOVVectorImage &other);
  BOVVectorImage(const BOVVectorImage &other);
  BOVVectorImage();
};

ostream &operator<<(ostream &os, const BOVVectorImage &si);

#endif
