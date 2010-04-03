/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef BOVTimeStepImage_h
#define BOVTimeStepImage_h

#include "BOVScalarImage.h"
#include "BOVVectorImage.h"
#include <vector>
using std::vector;

class BOVMetaData;

//*****************************************************************************
class BOVTimeStepImage
{
public:
  BOVTimeStepImage(MPI_Comm &comm, int stepId, BOVMetaData *metaData);
  ~BOVTimeStepImage();

  int GetNumberOfImages(){ return this->Scalars.size() + this->Vectors.size(); }

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


//*****************************************************************************
class BOVScalarImageIterator
{
public:
  ///
  BOVScalarImageIterator(BOVTimeStepImage *step)
       :
    Step(step),
    Idx(0),
    End(0)
      {
      this->End=this->Step->Scalars.size();
      }
  ///
  void Reset(){ this->Idx=0; }
  ///
  bool Ok(){ return this->Idx<this->End; }
  ///
  int Next(){
    if (this->Idx<this->End)
      {
      ++this->Idx;
      return this->Idx;
      }
      return 0;
    }
  ///
  MPI_File GetFile(){ return this->Step->Scalars[this->Idx]->GetFile(); }
  ///
  const char *GetName(){ return this->Step->Scalars[this->Idx]->GetName(); }
private:
  BOVTimeStepImage *Step;
  int Idx;
  int End;
private:
  BOVScalarImageIterator();
};

//*****************************************************************************
class BOVVectorImageIterator
{
public:
  ///
  BOVVectorImageIterator(BOVTimeStepImage *step)
       :
    Step(step),
    Idx(0),
    End(0)
      {
      this->End=this->Step->Vectors.size();
      }
  ///
  bool Ok(){ return this->Idx<this->End; }
  ///
  int Next(){
    if (this->Idx<this->End)
      {
      ++this->Idx;
      return this->Idx;
      }
      return 0;
    }
  ///
  MPI_File GetXFile(){ return this->Step->Vectors[this->Idx]->GetXFile(); }
  MPI_File GetYFile(){ return this->Step->Vectors[this->Idx]->GetYFile(); }
  MPI_File GetZFile(){ return this->Step->Vectors[this->Idx]->GetZFile(); }
  ///
  const char *GetName(){ return this->Step->Vectors[this->Idx]->GetName(); }
private:
  BOVTimeStepImage *Step;
  int Idx;
  int End;
private:
  BOVVectorImageIterator();
};

#endif
