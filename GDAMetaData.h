#ifndef GDAMetaData_h
#define GDAMetaData_h

#include "BOVMetaData.h"
#include "vtkType.h"

#include<vector> // STL include.
using namespace std;

/// Parser for SciberQuest GDA dataset format.
/**
Parser for SciberQuest's GDA dataset metadata format.
*/
class VTK_EXPORT GDAMetaData : public BOVMetaData
{
public:
  /// Constructor.
  GDAMetaData() : Ok(false) {};
  GDAMetaData(const GDAMetaData &other)
    {
    *this=other;
    }
  GDAMetaData &operator=(const GDAMetaData &other);
  /// Destructor.
  virtual ~GDAMetaData()
    {
    this->Close();
    }
  /// Virtual copy constructor. Create a new object and copy. return the copy.
  /// or 0 on error. Caller to delete.
  virtual BOVMetaData *Duplicate() const
    {
    GDAMetaData *other=new GDAMetaData;
    *other=*this;
    return other;
    }

  /// Open the metadata file, and parse metadata.
  /// return 0 on error.
  virtual int Open(const char *fileName);
  /// Return true if "Get" calls will succeed, i.e. there is an open metadata
  /// file.
  virtual bool IsOpen() const
    {
    return this->Ok;
    }
  /// Free any resources and set the object into a default
  /// state.
  /// return 0 on error.
  virtual int Close();

  /// Return the file extension used by the format for brick files.
  /// The BOV reader will make use of this in its pattern matching logic.
  virtual const char *GetBrickFileExtension() const
    {
    return "gda";
    }
  /// Return the file extension used by metadata files.
  //virtual const char *GetMetadataFileExtension() const =0;

  /// Print internal state.
  virtual void Print(ostream &os) const;
private:
  bool Ok;
  
};

#endif
