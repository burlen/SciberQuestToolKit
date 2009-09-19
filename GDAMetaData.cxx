#include "GDAMetaData.h"

#include<cstring>
#include<cstdlib>
#include<iostream>
#include<sstream>
#include<algorithm>

#ifndef WIN32
  #define PATH_SEP "/"
  #include<dirent.h>
#else
  #define PATH_SEP "\\"
#include "windirent.h"
#endif

#include "PrintUtils.h"

// Return 1 if a file is found with the prefix in the directory given by path.
//******************************************************************************
int Represented(const char *path, const char *prefix)
{
  size_t prefixLen=strlen(prefix);
  #ifndef NDEBUG
  if (prefix[prefixLen-1]!='_')
    {
    cerr << __LINE__ << " Error: prefix is expected to end with '_' but it does not." << endl;
    return 0;
    }
  #endif
  DIR *ds=opendir(path);
  if (ds)
    {
    struct dirent *de;
    while ((de=readdir(ds)))
      {
      char *fname=de->d_name;
      if (strncmp(fname,prefix,prefixLen)==0)
        {
        // Found at least one file beginning with the given prefix.
        closedir(ds);
        return 1;
        }
      }
    closedir(ds);
    }
  else
    {
    #ifndef NDEBUG
    cerr << __LINE__ << " Error: Failed to open the given directory. " << endl
         << path << endl;
    #endif
    }
  //We failed to find any files starting with the given prefix
  return 0;
}

// Collect the ids from a collection of files that start with
// the same prefix (eg. prefix_ID.ext). The prefix should include
// the underscore.
//*****************************************************************************
int GetSeriesIds(const char *path, const char *prefix, vector<int> &ids)
{
  size_t prefixLen=strlen(prefix);
  #ifndef NDEBUG
  if (prefix[prefixLen-1]!='_')
    {
    cerr << __LINE__ << " Error: prefix is expected to end with '_' but it does not." << endl;
    return 0;
    }
  #endif

  DIR *ds=opendir(path);
  if (ds)
    {
    struct dirent *de;
    while ((de=readdir(ds)))
      {
      char *fname=de->d_name;
      if (strncmp(fname,prefix,prefixLen)==0)
        {
        if (isdigit(fname[prefixLen]))
          {
          int id=atoi(fname+prefixLen);
          ids.push_back(id);
          }
        }
      }
    closedir(ds);
    sort(ids.begin(),ids.end());
    return 1;
    }
  else
    {
    #ifndef NDEBUG
    cerr << __LINE__ << " Error: Failed to open the given directory. " << endl
         << path << endl;
    #endif
    }
  //We failed to find any files starting with the given prefix
  return 0;
}


// Returns the path not including the file name and not
// including the final PATH_SEP. If PATH_SEP isn't found 
// then ".PATH_SEP" is returned.
//*****************************************************************************
string StripFileNameFromPath(const string fileName)
{
  size_t p;
  p=fileName.find_last_of(PATH_SEP);
  if (p==string::npos)
    {
    // current directory
    return "." PATH_SEP;
    }
  return fileName.substr(0,p); // TODO Why does this leak?
}

//*****************************************************************************
void ToLower(string &in)
{
  size_t n=in.size();
  for (size_t i=0; i<n; ++i)
    {
    in[i]=tolower(in[i]);
    }
}

// Parse a string for a "key", starting at offset "at" then 
// advance past the key and attempt to convert what follows
// in to a value of type "T". If the key isn't found, then 
// npos is returned otherwise the position imediately following
// the key is returned.
//*****************************************************************************
template <typename T>
size_t ParseValue(string &in,size_t at, string key, T &value)
{
  size_t p=in.find(key,at);
  if (p!=string::npos)
    {
    p+=key.size();
    istringstream valss(in.substr(p,10));
    valss >> value;
    }
  return p;
}

//-----------------------------------------------------------------------------
int GDAMetaData::Open(const char *fileName)
{
  if (this->IsOpen())
    {
    this->Close();
    }

  // Open
  ifstream metaFile(fileName);
  if (!metaFile.is_open())
    {
    #ifndef NDEBUG
    cerr << __LINE__ << " Error: Could not open " << fileName << endl;
    #endif
    return 0;
    }
  // Read
  metaFile.seekg(0,ios::end);
  size_t metaFileLen=metaFile.tellg();
  metaFile.seekg(0,ios::beg);
  char *metaDataBuffer=static_cast<char *>(malloc(metaFileLen+1));
  metaDataBuffer[metaFileLen]='\0';
  metaFile.read(metaDataBuffer,metaFileLen);
  string metaData(metaDataBuffer);
  free(metaDataBuffer); // TODO use string's memory directly
  // Parse
  ToLower(metaData);
  // All we are expecting are nx,ny, and nz. We'll assume the rest of the
  // details based on what we know of the simulation. Less than ideal but
  // until we get a better writer this will suffice.
  int nx,ny,nz;
  if ( ParseValue(metaData,0,"nx=",nx)==string::npos
    || ParseValue(metaData,0,"ny=",ny)==string::npos
    || ParseValue(metaData,0,"nz=",nz)==string::npos)
    {
    #ifndef NDEBUG
    cerr << __LINE__ << " Error: Parsing " << fileName 
         << " dimensions not found. Expected nx=N, ny=M, nz=P." << endl;
    #endif
    return 0;
    }
  vtkAMRBox domain(0,0,0,nx-1,ny-1,nz-1);
  this->SetDomain(domain);

  // FIXME!
  // assumptions, based on what we know of the SciberQuest code.
  // BTW origin and spacing default to: x0={0,0,0}, dx={1,1,1}

  // We expect the bricks to be in the same directory as the metadata file.
  this->SetPathToBricks(StripFileNameFromPath(fileName).c_str());
  // scalars ...
  int nArrays=0;
  const char *path=this->GetPathToBricks();
  if (Represented(path,"den_"))
    {
    this->AddScalar("den");
    ++nArrays;
    }
  if (Represented(path,"eta_")) 
    {
    this->AddScalar("eta");
    ++nArrays;
    }
  if (Represented(path,"tpar_"))
    {
    this->AddScalar("tpar");
    ++nArrays;
    }
  if (Represented(path,"tperp_"))
    {
    this->AddScalar("tperp");
    ++nArrays;
    }
  // vectors ...
  if (Represented(path,"bx_")
    && Represented(path,"by_")
    && Represented(path,"bz_"))
    {
    this->AddVector("b");
    ++nArrays;
    }
  if (Represented(path,"vix_")
    && Represented(path,"viy_")
    && Represented(path,"viz_"))
    {
    this->AddVector("vi");
    ++nArrays;
    }
  // We had to find at least one brick, otherwise we have problems.
  // As long as there is at least one brick, generate the series ids.
  if (nArrays)
    {
    const char *arrayName=this->GetArrayName(0);
    string prefix(arrayName);
    if (this->IsArrayVector(arrayName))
      {
      prefix+="x_";
      }
    else
      {
      prefix+="_";
      }
    GetSeriesIds(path,prefix.c_str(),this->TimeSteps);
    if (!this->TimeSteps.size())
      {
      #ifndef NDEBUG
      cerr << __LINE__ << " Error: Time series was not found in " << path << "."
          << " Expected files named according to the following convention \"array_time.ext\"" 
          << endl;
      #endif
      return 0;
      }
    }
  else
    {
    #ifndef NDEBUG
    cerr << __LINE__ << " Error: No bricks found in " << path << "."
        << " Expected bricks in the same directory as the metdata file."
        << endl;
    #endif
    return 0;
    }

  this->Ok=true;
  return 1;
}

//-----------------------------------------------------------------------------
int GDAMetaData::Close()
{
  this->Ok=false;
  BOVMetaData::Close();
  return 1;
}

//-----------------------------------------------------------------------------
GDAMetaData &GDAMetaData::operator=(const GDAMetaData &other)
{
  if (&other==this)
    {
    return *this;
    }
  this->Ok=other.Ok;
  this->BOVMetaData::operator=(other);
  return *this;
}

void GDAMetaData::Print(ostream &os) const
{
  os << "GDAMetaData: " << this << endl;
  os << "\tOk: " << this->Ok << endl;
  this->BOVMetaData::Print(os);
  os << endl;
}
