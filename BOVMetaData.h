#ifndef BOVMetaData_h
#define BOVMetaData_h

#if defined PV_3_4_BUILD
  #include "vtkAMRBox_3.7.h"
#else
  #include "vtkAMRBox.h"
#endif

#include "PrintUtils.h"
#include <cstdlib>
#include <map>
#include <vector>
#include <string>
using namespace std;

// These masks are used with array status methods.
// ACTIVE_BIT is set to indicate an array is to be read
// VECTOR_BIT is set to indicate an array is a vector, cleared for scalar.
// DIV_BIT is set to compute divergence
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
  /// Construct
  BOVMetaData(){}
  /// Copy from other.
  BOVMetaData &operator=(const BOVMetaData &other)
    {
    if (this==&other) return *this;
    this->PathToBricks=other.PathToBricks;
    this->Arrays=other.Arrays;
    this->TimeSteps=other.TimeSteps;
    this->Domain=other.Domain;
    this->Subset=other.Subset;
    this->Decomp=other.Decomp;
    return *this;
    }
  /// Copy construct.
  BOVMetaData(const BOVMetaData &other)
    {
    *this=other;
    }
  /// Virtual copy constructor. Create a new object and copy this into it. 
  /// return the copy or 0 on error. Caller to delete.
  virtual BOVMetaData *Duplicate() const=0;


  /// Open the metadata file, and parse metadata. return 0 on error.
  virtual int OpenDataset(const char *fileName)=0;
  /// Return true if "Get" calls will succeed, i.e. there is an open metadata
  /// file.
  virtual bool IsDatasetOpen() const =0;
  /// Close the currently open metatdata file, free any resources and set 
  /// the object into a default state. return 0 on error. Be sure to call
  /// BOVMetaData::CloseDataset().
  virtual int CloseDataset()
    {
    this->PathToBricks="";
    this->Domain.Invalidate();
    this->Subset.Invalidate();
    this->Decomp.Invalidate();
    this->Arrays.clear();
    this->TimeSteps.clear();
    return 1;
    }

  /// Set/Get the domain/subset/decomp of the dataset. These are initialized
  /// during OpenDataset(). The subset may be modified anytime there after by the user
  /// to reduce the amount of memory used during the actual read. Definitions:
  /// The domain is index space of the data on disk, the subset is the index space
  /// user identifies to read from disk, and the decomp is the index space assigned
  /// to this process to read. Note: Setting the domain also sets the subset and 
  /// decomp, and setting the subset also sets the decomp. 
  virtual vtkAMRBox GetDomain() const
    {
    return this->Domain;
    }
  virtual void SetDomain(const vtkAMRBox &domain)
    {
    this->Domain=domain;
    // If unitialized set the subset.
    if (this->Subset.Empty())
      {
      this->SetSubset(domain);
      }
    }
  virtual vtkAMRBox GetSubset() const
    {
    return this->Subset;
    }
  virtual void SetSubset(const vtkAMRBox &subset)
    {
    this->Subset=subset;
    // If unitialized set the decomp
    if (this->Decomp.Empty())
      {
      this->Decomp=subset;
      }
    }
  virtual vtkAMRBox GetDecomp() const
    {
    return this->Decomp;
    }
  virtual void SetDecomp(const vtkAMRBox &decomp)
    {
    this->Decomp=decomp;
    }


  /// Add a scalar array and set it's status to inactive.
  virtual void AddScalar(const char *name)
    {
    this->Arrays[name]=0;
    }
  /// Add a scalar array and set it's status to inactive.
  virtual void AddVector(const char *name)
    {
    this->Arrays[name]=VECTOR_BIT;
    }

  /// Indicate that the named array is a scalar.
  virtual void SetArrayTypeToScalar(const char *name)
    {
    this->Arrays[name] &= ~VECTOR_BIT;
    }
  /// Return true if the array is a scalar.
  virtual int IsArrayScalar(const char *name)
    {
    int status=this->Arrays[name];
    return (status&VECTOR_BIT)^VECTOR_BIT;
    }

  /// Indicate that the named array is a vector.
  virtual void SetArrayTypeToVector(const char *name)
    {
    this->Arrays[name] |= VECTOR_BIT;
    }
  /// Return true if the array is a vector.
  virtual int IsArrayVector(const char *name)
    {
    int status=this->Arrays[name];
    return status&VECTOR_BIT;
    }

  /// Activate the named array so thatit will be read.
  virtual void ActivateArray(const char *name)
    {
    this->Arrays[name] |= ACTIVE_BIT;
    }
  /// Deactivate the named array, it won't be read.
  virtual void DeactivateArray(const char *name)
    {
    this->Arrays[name] &= ~ACTIVE_BIT;
    }
  /// Test to see if an array is active.
  virtual int IsArrayActive(const char *name)
    {
    int status=this->Arrays[name];
    return status&ACTIVE_BIT;
    }

  /// Return the number of scalars in the dataset.
  virtual size_t GetNumberOfArrays() const
    {
    return this->Arrays.size();
    }

  /// Return the number of scalars in the dataset.
  virtual size_t GetNumberOfArrayFiles() const
    {
    size_t nFiles=0;
    map<string,int>::const_iterator it=this->Arrays.begin();
    map<string,int>::const_iterator end=this->Arrays.end();
    for (;it!=end; ++it)
      {
      nFiles+=(it->second&VECTOR_BIT?3:1);
      }
    return nFiles;
    }

  /// Return the requested array name.
  virtual const char *GetArrayName(size_t i) const
    {
    map<string,int>::const_iterator it=this->Arrays.begin();
    while(i--) it++;
    return it->first.c_str();
    }

  /// Return the number of time steps in the dataset, if a metadata
  /// file has been successfully opened.
  virtual size_t GetNumberOfTimeSteps() const
    {
    return this->TimeSteps.size();
    }
  /// Return the requested time step id, if a metadat file has been
  /// successfully opened.
  virtual int GetTimeStep(size_t i) const
    {
    return this->TimeSteps[i];
    }
  /// Return a pointer to the time steps array.
  virtual const int *GetTimeSteps() const
    {
    return &this->TimeSteps[0];
    }
  /// Return the requested time step id, if a metadat file has been
  /// successfully opened.
  virtual void AddTimeStep(int t)
    {
    this->TimeSteps.push_back(t);
    }

  /// Set/Get the directory where the bricks for th eopen dataset are
  /// located. Valid if a metadata file has been successfully opened.
  virtual void SetPathToBricks(const char *path)
    {
    this->PathToBricks=path;
    }
  virtual const char *GetPathToBricks() const
    {
    return this->PathToBricks.c_str();
    }

  /// Return the file extension used by the format for brick files.
  /// The BOV reader will make use of this in its pattern matching logic.
  virtual const char *GetBrickFileExtension() const =0;
  /// Return the file extension used by metadata files.
  //virtual const char *GetMetadataFileExtension() const =0;

  /// Print internal state.
  virtual void Print(ostream &os) const
    {
    os << "BOVMetaData: " << this << endl;
    os << "\tPathToBricks: " << this->PathToBricks << endl;
    os << "\tDomain: "; this->Domain.Print(os) << endl;
    os << "\tSubset: "; this->Subset.Print(os) << endl;
    os << "\tDecomp: "; this->Decomp.Print(os) << endl;
    os << "\tArrays: " << this->Arrays << endl;
    os << "\tTimeSteps: " << this->TimeSteps << endl;
    }

protected:
  string PathToBricks;            // path to the brick files.
  vtkAMRBox Domain;               // Dataset domain on disk.
  vtkAMRBox Subset;               // Subset of interst to read.
  vtkAMRBox Decomp;               // Part of the subset this process will read.
  map<string,int> Arrays;         // map of srray names to a status flag.
  vector<int> TimeSteps;          // Time values.
};

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
