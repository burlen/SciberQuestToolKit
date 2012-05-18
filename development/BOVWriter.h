/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/
#ifndef __BOVWriter_h
#define __BOVWriter_h

#include <mpi.h>
#include <vector>
using std::vector;
#include <string>
using std::string;

#include "SQExport.h"
#include "RefCountedPointer.h"
#include "BOVMetaData.h"

class vtkDataSet;
class vtkAlgorithm;
class BOVScalarImageIterator;
class BOVArrayImageIterator;
class BOVTimeStepImage;

/// Low level writer for BOV files with domain decomposition capability.
/**
Given a domain and a set of files writes subsets of the files into
vtkImageData objects point data.

Calls that return an int generally return 0 to indicate an error.
*/
class SQ_EXPORT BOVWriter : public RefCountedPointer
{
public:
  static BOVWriter *New(){ return new BOVWriter; }

  /**
  Safely copying the writer.
  */
  const BOVWriter &operator=(const BOVWriter &other);

  /**
  Set the controller that will be used during IO and
  communication operations. Typically it's COMM_WORLD.
  */
  void SetCommunicator(MPI_Comm comm);
  MPI_Comm GetCommunicator(){ return this->Comm; }

  /**
  Set the info object conatining the file hints.
  Optional. If not set INFO_NULL is used.
  */
  void SetHints(MPI_Info hints);

  /**
  Set the metadata object that will interpret the metadata file,
  a deep copy of the passed in object is made prior to returning.
  See BOVMetaData for interface details.
  */
  void SetMetaData(const BOVMetaData *metaData);

  /**
  Get the active metadata object. Use this to querry the open dataset.
  See BOVMetaData.
  */
  BOVMetaData *GetMetaData() const { return this->MetaData; }

  /**
  Open a dataset.
  */
  int Open(const char *fileName);

  /**
  Return's true if the dataset has been successfully opened.
  */
  bool IsOpen();

  /**
  Close the dataset, release any held resources.
  */
  int Close();

  /**
  Rank 0 writes metadata to disk.
  */
  int WriteMetaData();


  /**
  Open a specific time step.
  */
  BOVTimeStepImage *OpenTimeStep(int stepNo);
  void CloseTimeStep(BOVTimeStepImage *handle);

  /**
  Write the named set of arrays from disk.
  */
  int WriteTimeStep(
        const BOVTimeStepImage *handle,
        vtkDataSet *idds,
        vtkAlgorithm *exec=0);

  /**
  Print internal state.
  */
  void PrintSelf(ostream &os);

protected:
  BOVWriter();
  BOVWriter(const BOVWriter &other);
  ~BOVWriter();

private:
  /**
  Write the array from the specified file into point data in a single
  pass.
  */
  int WriteScalarArray(const BOVScalarImageIterator &it, vtkDataSet *grid);
  int WriteVectorArray(const BOVArrayImageIterator &it, vtkDataSet *grid);
  int WriteSymetricTensorArray(const BOVArrayImageIterator &it, vtkDataSet *grid);

private:
  BOVMetaData *MetaData;     // Object that knows how to interpret dataset.
  int ProcId;                // My process id.
  int NProcs;                // Number of processes.
  MPI_Comm Comm;             // Communicator handle
  MPI_Info Hints;            // MPI-IO file hints.
};

#endif

