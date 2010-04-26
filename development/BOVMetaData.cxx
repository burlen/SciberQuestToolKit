/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "BOVMetaData.h"




//*****************************************************************************
static
size_t size(int i)
{
  return sizeof(int);
}

//*****************************************************************************
static
int pack(int i, void *&stream)
{
  *((int *)stream)=i;
  ++((int*&)stream);

  return 1;
}

//*****************************************************************************
static
int unpack(int &i, void *&stream)
{
  i=*((int *)stream);
  ++((int*&)stream);

  return 1;
}



//*****************************************************************************
static
size_t size(const string &str)
{
  return sizeof(int)+str.size()*sizeof(char);
}

//*****************************************************************************
static
int pack(const string &str, void *&stream)
{
  // string length
  int strLen=str.size();
  *((int *)stream)=strLen;
  ++((int*&)stream);

  // string
  const char *cstr=str.c_str();
  for (int i=0; i<strLen; ++i)
    {
    *((char *)stream)=cstr[i];
    ++((char*&)stream);
    }

  return 1;
}

//*****************************************************************************
static
int unpack(string &str, void *&stream)
{
  int strLen=*((int *)stream);
  ++((int*&)stream);

  str.resize(strLen);
  str.assign((char *)stream,strLen);
  ((char *&)stream)+=strLen;

  return 1;
}

//*****************************************************************************
static
size_t size(vtkAMRBox &b)
{
  return 6*sizeof(int)+6*sizeof(double);
}

//*****************************************************************************
static
int pack(vtkAMRBox &b, void *&stream)
{
  b.GetDimensions((int *)stream);
  ((int *&)stream)+=6;

  b.GetGridSpacing((double *)stream);
  ((double *&)stream)+=3;

  b.GetDataSetOrigin((double *)stream);
  ((double *&)stream)+=3;
}

//*****************************************************************************
static
int unpack(vtkAMRBox &b, void *&stream)
{
  b.SetDimensions((int *)stream);
  ((int *&)stream)+=6;

  b.SetGridSpacing((double *)stream);
  ((double *&)stream)+=3;

  b.SetDataSetOrigin((double *)stream);
  ((double *&)stream)+=3;
}

//*****************************************************************************
static
size_t size(vector<int> &v)
{
  return sizeof(int)*(1+v.size());
}

//*****************************************************************************
static
int pack(vector<int> &v, void *&stream)
{
  int vLen=v.size();
  *((int *)stream)=vLen;
  ++((int*&)stream);

  for (int i=0; i<vLen; ++i)
    {
    *((int *)stream)=v[i];
    ++((int*&)stream);
    }

  return 1;
}

//*****************************************************************************
static
int unpack(vector<int> &v, void *&stream)
{
  int vLen=*((int *)stream);
  ++((int*&)stream);
  v.resize(vLen);

  for (int i=0; i<vLen; ++i)
    {
    v[i]=*((int *)stream);
    ++((int*&)stream);
    }

  return 1;
}


//*****************************************************************************
static
size_t size(map<string,int> &m)
{
  // map len, plus one int per map entry.
  size_t n=sizeof(int)*(1+m.size());

  map<string,int>::iterator it=m.begin();
  map<string,int>::iterator end=m.end();
  for (;it!=end; ++it)
    {
    // keys
    n+=size(it->first);
    }

  return n;
}

//*****************************************************************************
static
int pack(map<string,int> &m, void *&stream)
{
  // map size
  int mapLen=m.size();
  *((int *)stream)=mapLen;
  ++((int*&)stream);

  map<string,int>::iterator it=m.begin();
  map<string,int>::iterator end=m.end();
  for (;it!=end; ++it)
    {
    // string
    pack(it->first,stream);

    // value
    *((int *)stream)=it->second;
    ++((int*&)stream);
    }

  return 1;
}

//*****************************************************************************
static
int unpack(map<string,int> &m, void *&stream)
{
  int mapLen=*((int *)stream);
  ++((int*&)stream);

  for (int i=0; i<mapLen; ++i)
    {
    // string
    string str;
    unpack(str,stream);
    // value
    int val=*((int *)stream);
    ++((int*&)stream);

    m[str]=val;
    }

  return 1;
}

//-----------------------------------------------------------------------------
int BOVMetaData::Pack(void *&stream)
{
  size_t nBytes
    =size(this->IsOpen)
    +size(this->PathToBricks)
    +size(this->Domain)
    +size(this->Decomp)
    +size(this->Subset)
    +size(this->Arrays)
    +size(this->TimeSteps);

  void *pStream=malloc(nBytes);

  stream=pStream;
  pack(this->IsOpen,stream);
  pack(this->PathToBricks,stream);
  pack(this->Domain,stream);
  pack(this->Decomp,stream);
  pack(this->Subset,stream);
  pack(this->Arrays,stream);
  pack(this->TimeSteps,stream);
  stream=pStream;

  return nBytes;
}

//-----------------------------------------------------------------------------
int BOVMetaData::UnPack(void *stream)
{
  unpack(this->IsOpen,stream);
  unpack(this->PathToBricks,stream);
  unpack(this->Domain,stream);
  unpack(this->Decomp,stream);
  unpack(this->Subset,stream);
  unpack(this->Arrays,stream);
  unpack(this->TimeSteps,stream);
}

//-----------------------------------------------------------------------------
void BOVMetaData::Print(ostream &os) const
{
  os << "BOVMetaData: " << this << endl;
  os << "\tIsOpen: " << this->IsOpen << endl;
  os << "\tPathToBricks: " << this->PathToBricks << endl;
  os << "\tDomain: "; this->Domain.Print(os) << endl;
  os << "\tSubset: "; this->Subset.Print(os) << endl;
  os << "\tDecomp: "; this->Decomp.Print(os) << endl;
  os << "\tArrays: " << this->Arrays << endl;
  os << "\tTimeSteps: " << this->TimeSteps << endl;
}
