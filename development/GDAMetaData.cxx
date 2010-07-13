/*   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "GDAMetaData.h"
#include "GDAMetaDataKeys.h"

#include <iostream>
using std::ostream;
using std::endl;

#include <sstream>
using std::istringstream;

#include "PrintUtils.h"
#include "FsUtils.h"
#include "SQMacros.h"
#include "Tuple.hxx"

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
GDAMetaData::GDAMetaData()
{
  this->HasDipoleCenter=false;
  this->DipoleCenter[0]=
  this->DipoleCenter[1]=
  this->DipoleCenter[2]=-555.5;
}

//-----------------------------------------------------------------------------
int GDAMetaData::OpenDataset(const char *fileName)
{
  this->IsOpen=0;

  // Open
  ifstream metaFile(fileName);
  if (!metaFile.is_open())
    {
    sqErrorMacro(cerr,"Could not open " << fileName << ".");
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
  // We are expecting are nx,ny, and nz. We'll assume the rest of the
  // details based on what we know of the simulation.
  int nx,ny,nz;
  if ( ParseValue(metaData,0,"nx=",nx)==string::npos
    || ParseValue(metaData,0,"ny=",ny)==string::npos
    || ParseValue(metaData,0,"nz=",nz)==string::npos )
    {
    sqErrorMacro(cerr,
         << "Parsing " << fileName
         << " dimensions not found. Expected nx=N, ny=M, nz=P.");
    return 0;
    }

  CartesianExtent domain(0,nx-1,0,ny-1,0,nz-1);
  this->SetDomain(domain);

  double X0[3]={0.0,0.0,0.0};
  this->SetOrigin(X0);

  double dX[3]={1.0,1.0,1.0};
  this->SetSpacing(dX);

  // TODO
  // the following meta data enhancments are disabled until
  // I add the virtual pack/unpack methods, so that the process
  // that touches disk can actually stream them to the other
  // processes

  // // Look for the dipole center
  // double di_i,di_j,di_k;
  // if ( ParseValue(metaData,0,"i_dipole=",di_i)==string::npos
  //   || ParseValue(metaData,0,"j_dipole=",di_j)==string::npos
  //   || ParseValue(metaData,0,"k_dipole=",di_k)==string::npos)
  //   {
  //   // cerr << __LINE__ << " Warning: Parsing " << fileName
  //   //       << " dipole center not found." << endl;
  //   this->HasDipoleCenter=false;
  //   }
  // else
  //   {
  //   this->HasDipoleCenter=true;
  //   this->DipoleCenter[0]=di_i;
  //   this->DipoleCenter[1]=di_j;
  //   this->DipoleCenter[2]=di_k;
  //   }

  // double r_mp;
  // double r_obs_to_mp;
  // if ( ParseValue(metaData,0,"R_MP=",r_mp)==string::npos
  //   || ParseValue(metaData,0,"R_obstacle_to_MP=",r_obs_to_mp)==string::npos)
  //   {
  //   cerr << __LINE__ << " Warning: Parsing " << fileName 
  //         << " magnetopause dimension not found." << endl;
  //   this->CellSizeRe=-1.0;
  //   }
  // else
  //   {
  //   this->CellSizeRe=r_mp*r_obs_to_mp/100.0;
  //   }
  // double 
  // 
  //     i_dipole=100,
  //     j_dipole=128,
  //     k_dipole=128,
  //     R_MP=16.,
  //     R_obstacle_to_MP=0.57732,

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
  // 2d vector projections 
  if (Represented(path,"bpx_")
    && Represented(path,"bpy_")
    && Represented(path,"bpz_"))
    {
    this->AddVector("bp");
    ++nArrays;
    }
  if (Represented(path,"epx_")
    && Represented(path,"epy_")
    && Represented(path,"epz_"))
    {
    this->AddVector("ep");
    ++nArrays;
    }
  if (Represented(path,"vipx_")
    && Represented(path,"vipy_")
    && Represented(path,"vipz_"))
    {
    this->AddVector("vip");
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
      sqErrorMacro(cerr,
        << " Error: Time series was not found in " << path << "."
        << " Expected files named according to the following convention \"array_time.ext\".");
      return 0;
      }
    }
  else
    {
    sqErrorMacro(cerr,
      << " Error: No bricks found in " << path << "."
      << " Expected bricks in the same directory as the metdata file.");
    return 0;
    }

  this->IsOpen=1;

  return 1;
}

// this is removed because we don't currently need to free any reosurces
// when the file closes. I left it in case at a future time we need it.
// //-----------------------------------------------------------------------------
// int GDAMetaData::CloseDataset()
// {
//   this->IsOpen=0;
//   BOVMetaData::CloseDataset();
//   return 1;
// }


//-----------------------------------------------------------------------------
void GDAMetaData::PushPipelineInformation(vtkInformation *pinfo)
{
  if (this->HasDipoleCenter)
    {
    pinfo->Set(GDAMetaDataKeys::DIPOLE_CENTER(),this->DipoleCenter,3);
    }
  // pinfo->Set(GDAMetaDataKeys::CELL_SIZE_RE(),this->CellSizeRe);
}

//-----------------------------------------------------------------------------
GDAMetaData &GDAMetaData::operator=(const GDAMetaData &other)
{
  if (&other==this)
    {
    return *this;
    }
  this->BOVMetaData::operator=(other);
  return *this;
}

void GDAMetaData::Print(ostream &os) const
{
  os << "GDAMetaData:  " << this << endl;
  os << "\tDipole:     " << Tuple<double>(this->DipoleCenter,3) << endl;
  //os << "\tCellSizeRe: " << this->CellSizeRe << endl;

  this->BOVMetaData::Print(os);

  os << endl;
}
