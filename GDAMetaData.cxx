/*   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "GDAMetaData.h"

//#include<cstring>
//#include<cstdlib>
#include<iostream>
#include<sstream>
//#include<algorithm>

// #ifndef WIN32
//   #define PATH_SEP "/"
//   #include<dirent.h>
// #else
//   #define PATH_SEP "\\"
// #include "windirent.h"
// #endif

#include "PrintUtils.h"
#include "FsUtils.h"

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
int GDAMetaData::OpenDataset(const char *fileName)
{
  if (this->IsDatasetOpen())
    {
    this->CloseDataset();
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
  // origin and spacing default to: x0={0,0,0}, dx={1,1,1}

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
  if (Represented(path,"t_"))
    {
    this->AddScalar("t");
    ++nArrays;
    }
  if (Represented(path,"p_"))
    {
    this->AddScalar("p");
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
  if (Represented(path,"ex_")
    && Represented(path,"ey_")
    && Represented(path,"ez_"))
    {
    this->AddVector("e");
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
int GDAMetaData::CloseDataset()
{
  this->Ok=false;
  BOVMetaData::CloseDataset();
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
