#ifndef BOVReader_h
#define BOVReader_h

#include "BOVMetaData.h"

#include "mpi.h"

#if defined PV_3_4_BUILD
  #include "vtkAMRBox_3.7.h"
#else
  #include "vtkAMRBox.h"
#endif

#include <vector> // STL include
#include <string> // STL include
using std::vector;
using std::string;

class vtkImageData;
class vtkMultiBlockDataSet;
class vtkMultiProcessController;
class vtkAlgorithm;
class BOVScalarImageIterator;
class BOVVectorImageIterator;
class BOVTimeStepImage;

/// Low level reader for BOV files with domain decomposition capability.
/**
Given a domain and a set of files reads subsets of the files into
vtkImageData objects point data.

Calls that return an int generally return 0 to indicate an error.
*/
class VTK_EXPORT BOVReader
{
public:
  BOVReader();
  BOVReader(const BOVReader &other);
  ~BOVReader();

  // Description:
  // Safely copying the reader.
  const BOVReader &operator=(const BOVReader &other);

  // Description:
  // Set the controller that will be used during IO and
  // communication operations. Tyoically it's COMM_WORLD.
  void SetCommunicator(MPI_Comm comm);

  // Description:
  // Set the info object conatining the file hints.
  // Optional. If not set INFO_NULL is used.
  void SetHints(MPI_Info hints);

  // Description:
  // Set the metadata object that will interpret the metadata file,
  // a deep copy of the passed in object is made prior to returning.
  // See BOVMetaData for interface details.
  void SetMetaData(const BOVMetaData *metaData);
  // Description:
  // Get the active metadata object. Use this to querry the open dataset.
  // See BOVMetaData.
  BOVMetaData *GetMetaData() const { return this->MetaData; }

  // Description:
  // Open a dataset. During open meta data is parsed but no
  // heavy data is read.
  int Open(const char *fileName);

  // Description:
  // Return's true if the dataset has been successfully opened.
  bool IsOpen();

  // Description:
  // Close the dataset.
  int Close();

  // Description:
  // Set number of ghost cells to use with each sub-domain default 
  // is 1.
  int GetNumberOfGhostCells(){ return this->NGhost; }
  void SetNumberOfGhostCells(int nGhost){ this->NGhost=nGhost; }


  // Description:
  // Open a specific time step. This is done indepedently of the
  // read so that if running out of core only a single open is 
  // requried.
  BOVTimeStepImage *OpenTimeStep(int stepNo);
  void CloseTimeStep(BOVTimeStepImage *handle);

  // Description:
  // Read the named set of arrays from disk, use "Add" methods to add arrays
  // to be read.
  int ReadTimeStep(BOVTimeStepImage *handle, vtkImageData *idds, vtkAlgorithm *exec=0);
  int ReadMetaTimeStep(int stepNo, vtkImageData *idds, vtkAlgorithm *exec=0);

  // Description:
  // Print internal state.
  void PrintSelf(ostream &os);

private:
  // Description:
  // Read the array in the specified file and insert it into
  // point data.
  int ReadScalarArray(BOVScalarImageIterator *it, vtkImageData *grid);
  int ReadVectorArray(BOVVectorImageIterator *it, vtkImageData *grid);

  int ReadScalarArray(
        const char *fileName,
        const char *scalarName,
        vtkImageData *grid);
  int ReadVectorArray(
        const char *xFileName,
        const char *yFileName,
        const char *zFileName,
        const char *scalarName,
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
