/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef BOVMetaData_h
#define BOVMetaData_h

#include "vtkInformation.h"
#include "BinaryStream.hxx"
#include "CartesianExtent.h"

#include <cstdlib>
#include <map>
using std::map;
#include <vector>
using std::vector;
#include <string>
using std::string;

// These masks are used with array status methods.
// ACTIVE_BIT is set to indicate an array is to be read
// VECTOR_BIT is set to indicate an array is a vector, cleared for scalar.
#define ACTIVE_BIT 0x01
#define VECTOR_BIT 0x02

/// Interface to a BOV MetaData file.
/**
This class defines the interface to a BOV MetaData file, implement
this interface and fill the data structures during open to enable the
BOVReader to read your dataset.
*/
class VTK_EXPORT BOVMetaData
{
public:
  BOVMetaData();
  BOVMetaData(const BOVMetaData &other){ *this=other; }
  BOVMetaData &operator=(const BOVMetaData &other);

  /**
  Virtual copy constructor. Create a new object and copy this into it. 
  return the copy or 0 on error. Caller to delete.
  */
  virtual BOVMetaData *Duplicate() const=0;

  /// \Section FileOperations \@{
  /**
  Set/Get the directory where the bricks for th eopen dataset are
  located. Valid if a metadata file has been successfully opened.
  */
  void SetPathToBricks(const char *path) {  this->PathToBricks=path; }
  const char *GetPathToBricks() const { return this->PathToBricks.c_str(); }

  /**
  Return the file extension used by the format for brick files.
  The BOV reader will make use of this in its pattern matching logic.
  */
  virtual const char *GetBrickFileExtension() const =0;

  /**
  Return the file extension used by metadata files.
  */
  //virtual const char *GetMetadataFileExtension() const =0;


  /**
  Open the metadata file, and parse metadata. return 0 on error.
  */
  virtual int OpenDataset(const char *fileName)=0;

  /**
  Return true if "Get" calls will succeed, i.e. there is an open metadata
  file.
  */
  virtual int IsDatasetOpen() const { return this->IsOpen; }

  /**
  Close the currently open metatdata file, free any resources and set 
  the object into a default state. return 0 on error. Be sure to call
  BOVMetaData::CloseDataset().
  */
  virtual int CloseDataset();
  /// \@}


  /// \Section DomainSpecification \@{
  /**
  Set/Get the domain/subset/decomp of the dataset. These are initialized
  during OpenDataset(). The subset may be modified anytime there after by the user
  to reduce the amount of memory used during the actual read. Definitions:
  The domain is index space of the data on disk, the subset is the index space
  user identifies to read from disk, and the decomp is the index space assigned
  to this process to read. Note: Setting the domain also sets the subset and
  decomp, and setting the subset also sets the decomp.
  */
  void SetDomain(const CartesianExtent &domain);
  void SetSubset(const CartesianExtent &subset);
  void SetDecomp(const CartesianExtent &decomp);

  CartesianExtent GetDomain() const { return this->Domain; }
  CartesianExtent GetSubset() const { return this->Subset; }
  CartesianExtent GetDecomp() const { return this->Decomp; }

  /**
  Set/Get the dataset origin.
  */
  void SetOrigin(const double *origin);
  void SetOrigin(double x0, double y0, double z0);
  void GetOrigin(double *origin) const;
  double *GetOrigin(){ return this->Origin; }
  const double *GetOrigin() const { return this->Origin; }

  /**
  Set/Get the grid spacing.
  */
  void SetSpacing(const double *spacing);
  void SetSpacing(double dx, double dy, double dz);
  void GetSpacing(double *spacing) const;
  double *GetSpacing(){ return this->Spacing; }
  const double *GetSpacing() const { return this->Spacing; }
  /// \@}


  /// \Section ArrayModifiers \@{
  /**
  Add an array to the list of available arrays.
  */
  void AddScalar(const char *name){ this->Arrays[name]=0; }
  void AddVector(const char *name){ this->Arrays[name]=VECTOR_BIT; }
  /**
  Set the arry type.
  */
  void SetArrayTypeToScalar(const char *name){ this->Arrays[name]&=~VECTOR_BIT; }
  void SetArrayTypeToVector(const char *name){ this->Arrays[name]|=VECTOR_BIT; }
  /**
  Activate/Deactivate the named array so thatit will be read.
  */
  void ActivateArray(const char *name){ this->Arrays[name] |= ACTIVE_BIT; }
  void DeactivateArray(const char *name){ this->Arrays[name] &= ~ACTIVE_BIT; }
  /// \@}


  /// \Section ArrayQuerries \@{
  /**
  Query the array type.
  */
  int IsArrayScalar(const char *name)
    {
    int status=this->Arrays[name];
    return (status&VECTOR_BIT)^VECTOR_BIT;
    }
  int IsArrayVector(const char *name)
    {
    int status=this->Arrays[name];
    return status&VECTOR_BIT;
    }
  /**
  Query the named array's status
  */
  int IsArrayActive(const char *name)
    {
    int status=this->Arrays[name];
    return status&ACTIVE_BIT;
    }
  /**
  Return the number of scalars in the dataset.
  */
  size_t GetNumberOfArrays() const { return this->Arrays.size(); }

  /**
  Return the number of files comprising the dataset.
  */
  size_t GetNumberOfArrayFiles() const;

  /**
  Return the i-th array's name.
  */
  const char *GetArrayName(size_t i) const;
  /// \@}

  /// \Section TimeSupport \@{
  /**
  Return the requested time step id, if a metadat file has been
  successfully opened.
  */
  virtual void AddTimeStep(int t) { this->TimeSteps.push_back(t); }

  /**
  Return the number of time steps in the dataset, if a metadata
  file has been successfully opened.
  */
  size_t GetNumberOfTimeSteps() const { return this->TimeSteps.size(); }

  /**
  Return the requested time step id, if a metadat file has been
  successfully opened.
  */
  int GetTimeStep(size_t i) const { return this->TimeSteps[i]; }

  /**
  Return a pointer to the time steps array.
  */
  const int *GetTimeSteps() const { return &this->TimeSteps[0]; }
  /// \@}


  /**
  Implemantion's chance to add any specialized key,value pairs
  it needs into the pipeline information.
  */
  virtual void PushPipelineInformation(vtkInformation *pinfo){}

  /**
  Serialize the object into a byte stream  Returns the
  size in bytes of the stream. Or 0 in case of an error.
  */
  virtual void Pack(BinaryStream &str);

  /**
  Initiaslize the object froma byte stream (see also Serialize)
  returns 0 in case of an error.
  */
  virtual void UnPack(BinaryStream &str);


  /// Print internal state.
  virtual void Print(ostream &os) const;

private:
  friend ostream &operator<<(ostream &os, const BOVMetaData &md);

protected:
  int IsOpen;
  string PathToBricks;      // path to the brick files.
  double Origin[3];         // dataset origin
  double Spacing[3];        // grid spacing.
  CartesianExtent Domain;   // Dataset domain on disk.
  CartesianExtent Subset;   // Subset of interst to read.
  CartesianExtent Decomp;   // Part of the subset this process will read.
  map<string,int> Arrays;   // map of srray names to a status flag.
  vector<int> TimeSteps;    // Time values.
};

ostream &operator<<(ostream &os, const BOVMetaData &md);

// TODO develop workable traits model.
/*
/// Traits for data array types.
template<typename T> class ArrayTraits;
/// Float
template<> class ArrayTraits<float>
{
public:
  typedef vtkFloatArray vtkType;
  typedef float cppType;
};
/// Double
template<> class ArrayTraits<double>
{
public:
  typedef vtkDoubleArray vtkType;
  typedef double cppType;
};
/// and so on ...
*/
#endif
