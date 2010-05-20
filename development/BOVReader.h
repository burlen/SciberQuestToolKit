/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __BOVReader_h
#define __BOVReader_h

#include <mpi.h>
#include <vector>
using std::vector;
#include <string>
using std::string;

#include "RefCountedPointer.h"
#include "BOVMetaData.h"

class vtkImageData;
class vtkMultiBlockDataSet;
class vtkMultiProcessController;
class vtkAlgorithm;
class BOVScalarImageIterator;
class BOVVectorImageIterator;
class BOVTimeStepImage;
class CartesianDataBlockIODescriptor;

/// Low level reader for BOV files with domain decomposition capability.
/**
Given a domain and a set of files reads subsets of the files into
vtkImageData objects point data.

Calls that return an int generally return 0 to indicate an error.
*/
class VTK_EXPORT BOVReader : public RefCountedPointer
{
public:
  static BOVReader *New(){ return new BOVReader; }

  /**
  Safely copying the reader.
  */
  const BOVReader &operator=(const BOVReader &other);

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
  Open a dataset. During open meta data is parsed but no
  heavy data is read.
  */
  int Open(const char *fileName);

  /**
  Return's true if the dataset has been successfully opened.
  */
  bool IsOpen();

  /**
  Close the dataset.
  */
  int Close();

  /**
  Set number of ghost cells to use with each sub-domain default 
  is 1.
  */
  int GetNumberOfGhostCells(){ return this->NGhost; }
  void SetNumberOfGhostCells(int nGhost){ this->NGhost=nGhost; }


  /**
  Open a specific time step. This is done indepedently of the
  read so that if running out of core only a single open is 
  requried.
  */
  BOVTimeStepImage *OpenTimeStep(int stepNo);
  void CloseTimeStep(BOVTimeStepImage *handle);

  /**
  Read the named set of arrays from disk, use "Add" methods to add arrays
  to be read.
  */
  int ReadTimeStep(
        const BOVTimeStepImage *handle,
        vtkImageData *idds,
        vtkAlgorithm *exec=0);

  int ReadTimeStep(
        const BOVTimeStepImage *hanlde,
        const CartesianDataBlockIODescriptor *descr,
        vtkImageData *grid,
        vtkAlgorithm *exec=0);

  int ReadMetaTimeStep(int stepNo, vtkImageData *idds, vtkAlgorithm *exec=0);

  /**
  Print internal state.
  */
  void PrintSelf(ostream &os);

protected:
  BOVReader();
  BOVReader(const BOVReader &other);
  ~BOVReader();

private:
  /**
  Read the array from the specified file into point data in a single
  pass.
  */
  int ReadScalarArray(const BOVScalarImageIterator &it, vtkImageData *grid);
  int ReadVectorArray(const BOVVectorImageIterator &it, vtkImageData *grid);

  /**
  Read the array from the specified file into point data in multiple
  passes described by the IO descriptor.
  */
  int ReadScalarArray(
        const BOVScalarImageIterator &fhit,
        const CartesianDataBlockIODescriptor *descr,
        vtkImageData *grid);

  int ReadVectorArray(
        const BOVVectorImageIterator &fhit,
        const CartesianDataBlockIODescriptor *descr,
        vtkImageData *grid);

private:
  BOVMetaData *MetaData;     // Object that knows how to interpret dataset.
  int NGhost;                // Number of ghost nodes, default is 1.
  int ProcId;                // My process id.
  int NProcs;                // Number of processes.
  MPI_Comm Comm;             // Communicator handle
  MPI_Info Hints;            // MPI-IO file hints.
};

#endif
