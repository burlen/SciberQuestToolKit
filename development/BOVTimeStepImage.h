/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __BOVTimeStepImage_h
#define __BOVTimeStepImage_h

#include "BOVScalarImage.h"
#include "BOVVectorImage.h"

#include <vector>
using std::vector;

class BOVMetaData;

/// Handle to single timestep in a dataset.
/**
Handle to single timestep in a dataset. A collection of 
handles to the scalar and vector data that comprise the 
time step.
*/
class BOVTimeStepImage
{
public:
  BOVTimeStepImage(
      MPI_Comm comm,
      MPI_Info hints,
      int stepId,
      BOVMetaData *metaData);
  ~BOVTimeStepImage();

  int GetNumberOfImages() const { return this->Scalars.size()+this->Vectors.size(); }

private:
  vector<BOVScalarImage*> Scalars;
  vector<BOVVectorImage*> Vectors;
private:
  BOVTimeStepImage();
  BOVTimeStepImage(const BOVTimeStepImage&);
  const BOVTimeStepImage &operator=(const BOVTimeStepImage &);
private:
  friend ostream &operator<<(ostream &os, const BOVTimeStepImage &si);
  friend class BOVScalarImageIterator;
  friend class BOVVectorImageIterator;
};

ostream &operator<<(ostream &os, const BOVTimeStepImage &si);

#endif
