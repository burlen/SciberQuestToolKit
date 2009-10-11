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

/// Low level reader for BOV files with domain decomposition capability.
/**
Given a domain and a set of files reads subsets of the files into
vtkImageData objects point data.

Calls that return an int generally return 0 to indicate an error.
*/
class VTK_EXPORT BOVReader
{
public:
  BOVReader() : MetaData(NULL), Comm(MPI_COMM_NULL) {
    this->Clear();
    }
  BOVReader(const BOVReader &other) {
    *this=other;
    }
  ~BOVReader(){
    this->SetMetaData(NULL);
    this->Clear();
    }

  /// Safely copying the reader.
  const BOVReader &operator=(const BOVReader &other);

  /// Identify this among a number of processes.
  int SetController(vtkMultiProcessController *cont);
  void SetCommunicator(MPI_Comm comm){
    this->Comm=comm;
    }
  void SetProcId(int procId){
    this->ProcId=procId;
    }
  void SetNProcs(int nProcs){
    this->NProcs=nProcs;
    }

  /// Set the metadata object that will interpret the metadata file,
  /// a deep copy of the passed in object is made prior to returning.
  /// See BOVMetaData for interface details.
  void SetMetaData(const BOVMetaData *metaData);
  /// Get the active metadata object. Use this to querry the open dataset.
  /// See BOVMetaData.
  BOVMetaData *GetMetaData() const{
    return this->MetaData;
    }
  /// Open a dataset. Pass the dataset's metadata file in. If this call succeeeds
  /// then ...
  int Open(const char *fileName){
    return this->MetaData && fileName && this->MetaData->Open(fileName);
    }
  /// Return's true if the dataset has been successfully opened.
  bool IsOpen(){
    if (this->MetaData)
      {
      return this->MetaData->IsOpen();
      }
    return false;
    }
  /// Close the dataset.
  int Close(){
    this->ClearDecomp();
    return this->MetaData && this->MetaData->Close();
    }

  /// Set number of ghost cells to use with each sub-domain default 
  /// is 1.
  int GetNumberOfGhostCells() const {
    return this->NGhost;
    }
  void SetNumberOfGhostCells(int nGhost){
    this->NGhost=nGhost;
    }
  /// Decompose the domain into a slab for each processor.
  int DecomposeDomain(vtkMultiBlockDataSet *mbds);

  /// Read the named set of arrays from disk, use "Add" methods to add arrays
  /// to be read.
  int ReadTimeStep(int stepNo, vtkMultiBlockDataSet *mbds);
  int ReadTimeStep(int stepNo, vtkImageData *idds, vtkAlgorithm *exec=0);
  int ReadMetaTimeStep(int stepNo, vtkImageData *idds, vtkAlgorithm *exec=0);

  /// Print internal state.
  void Print(ostream &os);

private:
  /// Place the object in a default state.
  void Clear();
  /// Free any vtk objects we have used.
  void ClearDecomp(){
    this->Blocks.clear();
    }
  /// Read the array in the specified file and insert it into
  /// point data.
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
  BOVMetaData *MetaData;    /// Object that knows how to interpret dataset.
  int NGhost;               /// Number of ghost nodes, default is 1.
  int ProcId;               /// My process id.
  int NProcs;               /// Number of processes.
  vector<vtkAMRBox> Blocks; /// Decomposed domain.
  MPI_Comm Comm;            /// Communicator handle
};

#endif
