/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "BOVMetaData.h"

#include "Tuple.hxx"
#include "PrintUtils.h"


//-----------------------------------------------------------------------------
BOVMetaData::BOVMetaData()
     :
  IsOpen(0)
{
  this->Origin[0]=
  this->Origin[1]=
  this->Origin[2]=0.0;

  this->Spacing[0]=
  this->Spacing[1]=
  this->Spacing[2]=1.0;
}

//-----------------------------------------------------------------------------
BOVMetaData &BOVMetaData::operator=(const BOVMetaData &other)
{
  if (this==&other) 
    {
    return *this;
    }

  this->IsOpen=other.IsOpen;
  this->PathToBricks=other.PathToBricks;
  this->Arrays=other.Arrays;
  this->TimeSteps=other.TimeSteps;
  this->Domain=other.Domain;
  this->Subset=other.Subset;
  this->Decomp=other.Decomp;
  this->SetOrigin(other.GetOrigin());
  this->SetSpacing(other.GetSpacing());

  return *this;
}

//-----------------------------------------------------------------------------
int BOVMetaData::CloseDataset()
{
  this->IsOpen=0;
  this->PathToBricks="";
  this->Domain.Clear();
  this->Subset.Clear();
  this->Decomp.Clear();
  this->Arrays.clear();
  this->TimeSteps.clear();
  return 1;
}

//-----------------------------------------------------------------------------
void BOVMetaData::SetDomain(const CartesianExtent &domain)
{
  this->Domain=domain;

  if (this->Subset.Empty())
    {
    this->SetSubset(domain);
    }
}

//-----------------------------------------------------------------------------
void BOVMetaData::SetSubset(const CartesianExtent &subset)
{
  this->Subset=subset;

  if (this->Decomp.Empty())
    {
    this->Decomp=subset;
    }
}

//-----------------------------------------------------------------------------
void BOVMetaData::SetDecomp(const CartesianExtent &decomp)
{
  this->Decomp=decomp;
}

//-----------------------------------------------------------------------------
void BOVMetaData::SetOrigin(const double *origin)
{
  this->Origin[0]=origin[0];
  this->Origin[1]=origin[1];
  this->Origin[2]=origin[2];
}

//-----------------------------------------------------------------------------
void BOVMetaData::SetOrigin(double x0, double y0, double z0)
{
  double X0[3]={x0,y0,z0};
  this->SetOrigin(X0);
}

//-----------------------------------------------------------------------------
void BOVMetaData::GetOrigin(double *origin) const
{
  origin[0]=this->Origin[0];
  origin[1]=this->Origin[1];
  origin[2]=this->Origin[2];
}

//-----------------------------------------------------------------------------
void BOVMetaData::SetSpacing(const double *spacing)
{
  this->Spacing[0]=spacing[0];
  this->Spacing[1]=spacing[1];
  this->Spacing[2]=spacing[2];
}

//-----------------------------------------------------------------------------
void BOVMetaData::SetSpacing(double dx, double dy, double dz)
{
  double dX[3]={dx,dy,dz};
  this->SetSpacing(dX);
}

//-----------------------------------------------------------------------------
void BOVMetaData::GetSpacing(double *spacing) const
{
  spacing[0]=this->Spacing[0];
  spacing[1]=this->Spacing[1];
  spacing[2]=this->Spacing[2];
}

//-----------------------------------------------------------------------------
size_t BOVMetaData::GetNumberOfArrayFiles() const
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

//-----------------------------------------------------------------------------
const char *BOVMetaData::GetArrayName(size_t i) const
{
  map<string,int>::const_iterator it=this->Arrays.begin();
  while(i--) it++;
  return it->first.c_str();
}

//-----------------------------------------------------------------------------
void BOVMetaData::Pack(BinaryStream &os)
{
  os.Pack(this->IsOpen);
  os.Pack(this->PathToBricks);
  os.Pack(this->Domain.GetData(),6);
  os.Pack(this->Decomp.GetData(),6);
  os.Pack(this->Subset.GetData(),6);
  os.Pack(this->Origin,3);
  os.Pack(this->Spacing,3);
  os.Pack(this->Arrays);
  os.Pack(this->TimeSteps);
}

//-----------------------------------------------------------------------------
void BOVMetaData::UnPack(BinaryStream &is)
{
  is.UnPack(this->IsOpen);
  is.UnPack(this->PathToBricks);
  is.UnPack(this->Domain.GetData(),6);
  is.UnPack(this->Decomp.GetData(),6);
  is.UnPack(this->Subset.GetData(),6);
  is.UnPack(this->Origin,3);
  is.UnPack(this->Spacing,3);
  is.UnPack(this->Arrays);
  is.UnPack(this->TimeSteps);
}

//-----------------------------------------------------------------------------
void BOVMetaData::Print(ostream &os) const
{
  os << "BOVMetaData: " << this << endl;
  os << "\tIsOpen: " << this->IsOpen << endl;
  os << "\tPathToBricks: " << this->PathToBricks << endl;
  os << "\tDomain: " << this->Domain << endl;
  os << "\tSubset: " << this->Subset << endl;
  os << "\tDecomp: " << this->Decomp << endl;
  os << "\tArrays: " << this->Arrays << endl;
  os << "\tTimeSteps: " << this->TimeSteps << endl;
}

//-----------------------------------------------------------------------------
ostream &operator<<(ostream &os, const BOVMetaData &md)
{
  md.Print(os);
  return os;
}


