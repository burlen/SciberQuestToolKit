/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __BOVVectorImageIterator_h
#define __BOVVectorImageIterator_h

#include "BOVTimeStepImage.h"
#include "BOVVectorImage.h"

#include <vector>
using std::vector;

/// Iterator for a collection of vector handles.
class BOVVectorImageIterator
{
public:
  BOVVectorImageIterator(const BOVTimeStepImage *step)
       :
    Step(step),
    Idx(0),
    End(0)
      {
      this->End=this->Step->Vectors.size();
      }

  /**
  Will evaluate true during the traversal.
  */
  bool Ok() const { return this->Idx<this->End; }

  /**
  Advance the iterator.
  */
  int Next()
    {
    if (this->Idx<this->End)
      {
      ++this->Idx;
      return this->Idx;
      }
      return 0;
    }

  /**
  Get the component file names.
  */
  MPI_File GetXFile() const { return this->Step->Vectors[this->Idx]->GetXFile(); }
  MPI_File GetYFile() const { return this->Step->Vectors[this->Idx]->GetYFile(); }
  MPI_File GetZFile() const { return this->Step->Vectors[this->Idx]->GetZFile(); }
  MPI_File GetComponentFile(int comp) const
    {
    switch (comp)
      {
      case 0:
        return this->GetXFile();
        break;

      case 1:
        return this->GetYFile();
        break;

      case 2:
        return this->GetZFile();
        break;
      }
    return 0;
    }

  /**
  Get the array name.
  */
  const char *GetName() const { return this->Step->Vectors[this->Idx]->GetName(); }

private:
  /// \Section NotImplemented \@{
  BOVVectorImageIterator();
  BOVVectorImageIterator(const BOVVectorImageIterator &);
  void operator=(const BOVVectorImageIterator &);
  /// \@}

private:
  const BOVTimeStepImage *Step;
  int Idx;
  int End;
};

#endif
