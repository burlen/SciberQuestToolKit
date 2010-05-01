/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "BOVTimeStepImage.h"
#include "BOVMetaData.h"

#include <sstream>
#include <string>
#include <iostream>
using namespace std;

#ifdef WIN32
  #define PATH_SEP "\\"
#else
  #define PATH_SEP "/"
#endif

//-----------------------------------------------------------------------------
BOVTimeStepImage::BOVTimeStepImage(
      MPI_Comm comm,
      MPI_Info hints,
      int stepIdx,
      BOVMetaData *metaData)
{
  ostringstream seriesExt;
  seriesExt << "_" << stepIdx << "." << metaData->GetBrickFileExtension();
  // Open each array.
  size_t nArrays=metaData->GetNumberOfArrays();
  for (size_t i=0; i<nArrays; ++i)
    {
    const char *arrayName=metaData->GetArrayName(i);
    // skip inactive arrays
    if (!metaData->IsArrayActive(arrayName))
      {
      continue;
      }
    // scalar
    if (metaData->IsArrayScalar(arrayName))
      {
      // deduce the file name from the following convention: 
      // arrayname_step.ext
      ostringstream fileName;
      fileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << seriesExt.str();

      // open
      BOVScalarImage *scalar
        = new BOVScalarImage(comm,hints,fileName.str().c_str(),arrayName);

      this->Scalars.push_back(scalar);
      }
    // vector
    else
    if (metaData->IsArrayVector(arrayName))
      {
      // deduce the file name from the following convention: 
      // arrayname{x,y,z}_step.ext
      ostringstream xFileName,yFileName,zFileName;
      xFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "x" << seriesExt.str();
      yFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "y" << seriesExt.str();
      zFileName << metaData->GetPathToBricks() << PATH_SEP << arrayName << "z" << seriesExt.str();

      // open
      BOVVectorImage *vector
        = new BOVVectorImage(
            comm,
            hints,
            xFileName.str().c_str(),
            yFileName.str().c_str(),
            zFileName.str().c_str(),
            arrayName);

      this->Vectors.push_back(vector);
      }
    // other ?
    else
      {
      cerr << __LINE__ << " Error: bad array type for array " << arrayName << "." << endl; 
      }
    }
}

//-----------------------------------------------------------------------------
BOVTimeStepImage::~BOVTimeStepImage()
{
  int nScalars=this->Scalars.size();
  for (int i=0; i<nScalars; ++i)
    {
    delete this->Scalars[i];
    }

  int nVectors=this->Vectors.size();
  for (int i=0; i<nVectors; ++i)
    {
    delete this->Vectors[i];
    }
}

//-----------------------------------------------------------------------------
ostream &operator<<(ostream &os, const BOVTimeStepImage &si)
{
  os << "Scalars:" << endl;
  int nScalars=si.Scalars.size();
  for (int i=0; i<nScalars; ++i)
    {
    os << *si.Scalars[i] << endl;
    }

  os << "Vectors:" << endl;
  int nVectors=si.Vectors.size();
  for (int i=0; i<nVectors; ++i)
    {
    os << *si.Vectors[i] << endl;
    }

  return os;
}
