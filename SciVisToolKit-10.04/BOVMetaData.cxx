// #include "BOVMetaData.h"
// 
// 
// //-----------------------------------------------------------------------------
// void DeepCopy(const BOVMetaData &other)
// {
//   this->PathToBricks=other.PathToBricks;
//   this->Scalars=other.Scalars;
//   this->Vectors=other.Vectors;
//   this->TimeSteps=other.TimeSteps;
//   this->Domain=other.Domain;
// }
// 
// //-----------------------------------------------------------------------------
// void Print(ostream &os)
// {
//   os << "\tPathToBricks: " << this->PathToBricks << endl;
//   os << "\tDomain: "; this->Domain.Print(os) << endl;
//   os << "\tScalars: " << this->ScalarName << endl;
//   os << "\tVectors: " << this->VectorName << endl;
//   os << "\tTimeSteps: " << this->TimeSteps << endl;
// }
// 
// //-----------------------------------------------------------------------------
// virtual int Close()
// {
//   this->PathToBricks="";
//   this->Domain.Invalidate();
//   this->Scalars.clear();
//   this->Vectors.clear();
//   this->TimeSteps.clear();
//   return 1;
// }
// 
// /// Add a scalar array and set it's status to inactive.
// virtual void AddScalar(const char *name)
//   {
//   this->Scalars[name]=0;
//   }
// /// Return the number of scalars in the dataset, if a
// /// metadata file has been successfully opened.
// virtual size_t GetNumberOfScalars() const
//   {
//   return this->Scalars.size();
//   }
// /// Return the requested scalar array name, if a
// /// metadata file has been successfully opened.
// virtual const char *GetScalarName(size_t i) const
//   {
//   map<string,int>::iterator it=this->Scalars.begin();
//   it+=i;
//   return it->first.c_str();
//   }
// /// Return the status of the array, 0 is returned for inactive.
// /// Valid if a metadata file has been successfully opened.
// virtual int GetScalarStatus(size_t i) const
//   {
//   map<string,int>::iterator it=this->Scalars.begin();
//   it+=i;
//   return it->second.c_str();
//   }
// virtual int GetScalarStatus(const char *name) const
//   {
//   return this->Scalars[name];
//   }
// /// Set the status of the array, 0 is used to make the array inactive.
// /// Valid if a metadata file has been successfully opened.
// virtual int SetScalarStatus(const char *name, int value)
//   {
//   this->Scalars[name]=value;
//   }
// /// Return the VTK data type of the requested scalar
// /// array, if a metadata file has been successfully opened.
// virtual int GetScalarDataType(size_t i) const
//   {
//   return VTK_FLOAT;
//   }
// 
// /// Add a scalar array and set it's status to inactive.
// virtual void AddVector(const char *name)
//   {
//   this->Vectors[name]=0;
//   }
// /// Return the number of scalars in the dataset, if a
// /// metadata file has been successfully opened.
// virtual size_t GetNumberOfVectors() const
//   {
//   return this->Vectors.size();
//   }
// /// Return the requested scalar array name, if a
// /// metadata file has been successfully opened.
// virtual const char *GetVectorName(size_t i) const
//   {
//   map<string,int>::iterator it=this->Vectors.begin();
//   it+=i;
//   return it->first.c_str();
//   }
// /// Return the status of the array, 0 is returned for inactive.
// /// Valid if a metadata file has been successfully opened.
// virtual int GetVectorStatus(size_t i) const
//   {
//   map<string,int>::iterator it=this->Vectors.begin();
//   it+=i;
//   return it->second.c_str();
//   }
// virtual int GetVectorStatus(const char *name) const
//   {
//   return this->Vectors[name];
//   }
// /// Set the status of the array, 0 is used to make the array inactive.
// /// Valid if a metadata file has been successfully opened.
// virtual int SetVectorStatus(const char *name, int value)
//   {
//   this->Vectors[name]=value;
//   }
// /// Return the VTK data type of the requested scalar
// /// array, if a metadata file has been successfully opened.
// virtual int GetVectorDataType(size_t i) const
//   {
//   return VTK_FLOAT;
//   }
// 
// /// Return the number of time steps in the dataset, if a metadata
// /// file has been successfully opened.
// virtual size_t GetNumberOfTimeSteps() const
//   {
//   return this->TimeSteps.size();
//   }
// /// Return the requested time step id, if a metadat file has been
// /// successfully opened.
// virtual int GetTimeStep(size_t i) const
//   {
//   return this->TimeSteps[i];
//   }
// /// Return the requested time step id, if a metadat file has been
// /// successfully opened.
// virtual void AddTimeStep(int t)
//   {
//   this->TimeSteps.push_back(t);
//   }
// 
// /// Set/Get the directory where the bricks for th eopen dataset are
// /// located. Valid if a metadata file has been successfully opened.
// virtual void SetPathToBricks(const char *path)
//   {
//   this->PathToBricks=path;
//   }
// virtual const char *GetPathToBricks() const
//   {
//   return this->PathToBricks.c_str();
//   }
// 
// /// Return the file extension used by the format for brick files.
// /// The BOV reader will make use of this in its pattern matching logic.
// virtual const char *GetBrickFileExtension() const =0;
// /// Return the file extension used by metadata files.
// //virtual const char *GetMetadataFileExtension() const =0;
// 
// /// Print internal state.
// virtual void Print(ostream &os) const =0;
