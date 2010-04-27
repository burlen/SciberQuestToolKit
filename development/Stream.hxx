/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __Stream_hxx
#define __Stream_hxx

#include "vtkAMRBox.h"
#include <string>
using std::string;
#include <map>
using std::map;
#include <vector>
using std::vector;

class VTK_EXPORT Stream
{
public:
  Stream();
  Stream(const Stream &s);
  ~Stream();
  const Stream &operator=(const Stream &other);

  // Description:
  // Release all resources, set to a uninitialized state.
  void Clear();

  // Description:
  // Alolocate nBytes for the stream.
  void Resize(int nBytes);

  // Description:
  // Add nBytes to the stream.
  void Grow(int nBytes);

  // Description:
  // Get apointer to the stream internal representation.
  char *GetData(){ return this->Data; }

  // Description:
  // Get the size of the data in the stream.
  size_t GetSize(){ return this->DataP-this->Data; }

  // Description:
  // Set the stream position to the head of the strem
  void Rewind()
    {
    this->DataP=this->Data;
    }

  // Description:
  // Insert/Extract various types to/from the stream.
  template <typename T> void Pack(T *val);
  template <typename T> void Pack(T val);
  template <typename T> void UnPack(T &val);
  template <typename T> void Pack(const T *val, int n);
  template <typename T> void UnPack(T *val, int n);
  // specializations
  void Pack(const string &str);
  void UnPack(string &str);
  void Pack(vtkAMRBox &b);
  void UnPack(vtkAMRBox &b);
  void Pack(map<string,int> &m);
  void UnPack(map<string,int> &m);
  template<typename T> void Pack(vector<T> &v);
  template<typename T> void UnPack(vector<T> &v);


private:


private:
  size_t Size;
  char *Data;
  char *DataP;
};


//-----------------------------------------------------------------------------
inline
Stream::Stream()
     :
  Size(0),
  Data(0),
  DataP(0)
    {
    // this->Size=64;
    // this->Data=(char*)malloc(this->Size);
    // this->DataP=this->Data;
    // for (int i=0; i<this->Size; ++i)
    //   {
    //   this->Data[i]='\0';
    //   }
    }

//-----------------------------------------------------------------------------
inline
Stream::~Stream()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
inline
Stream::Stream(const Stream &other)
{
  *this=other;
}

//-----------------------------------------------------------------------------
inline
const Stream &Stream::operator=(const Stream &other)
{
  if (&other==this) return *this;

  this->Clear();

  this->Resize(other.Size);

  this->DataP=other.DataP;

  for (size_t i=0; i<other.Size; ++i)
    {
    this->Data[i]=other.Data[i];
    }

  return *this;
}

//-----------------------------------------------------------------------------
inline
void Stream::Clear()
{
  free(this->Data);
  this->Data=0;
  this->DataP=0;
  this->Size=0;
}

//-----------------------------------------------------------------------------
inline
void Stream::Resize(int nBytes)
{
  char *origData=this->Data;
  this->Data=(char *)realloc(this->Data,nBytes);

  // update the stream pointer
  if (this->Data!=origData)
    {
    this->DataP=this->Data+(this->DataP-origData);
    }

  this->Size=nBytes;
}

//-----------------------------------------------------------------------------
inline
void Stream::Grow(int nBytes)
{
  this->Resize(this->Size+nBytes);
}

//-----------------------------------------------------------------------------
template <typename T>
void Stream::Pack(T *val)
{
  cerr << "Error: Packing a pointer." << endl;
}

//-----------------------------------------------------------------------------
template <typename T>
void Stream::Pack(T val)
{
  this->Grow(sizeof(T));

  *((T *)this->DataP)=val;

  this->DataP+=sizeof(T);
}

//-----------------------------------------------------------------------------
template <typename T>
void Stream::UnPack(T &val)
{
  val=*((T *)this->DataP);

  this->DataP+=sizeof(T);
}

//-----------------------------------------------------------------------------
template <typename T>
void Stream::Pack(const T *val, int n)
{
  const int nBytes=n*sizeof(T);

  this->Grow(nBytes);

  for (int i=0; i<n; ++i,this->DataP+=sizeof(T))
    {
    *((T *)this->DataP)=val[i];
    }
}

//-----------------------------------------------------------------------------
template <typename T>
void Stream::UnPack(T *val, int n)
{
  for (int i=0; i<n; ++i, this->DataP+=sizeof(T))
    {
    val[i]=*((T *)this->DataP);
    }
}

//-----------------------------------------------------------------------------
inline
void Stream::Pack(const string &str)
{
  int strLen=str.size();
  this->Pack(strLen);
  this->Pack(str.c_str(),strLen);
}

//-----------------------------------------------------------------------------
inline
void Stream::UnPack(string &str)
{
  int strLen=0;
  this->UnPack(strLen);

  str.resize(strLen);
  str.assign(this->DataP,strLen);

  this->DataP+=strLen;
}

//-----------------------------------------------------------------------------
inline
void Stream::Pack(vtkAMRBox &b)
{
  int dims[6];
  b.GetDimensions(dims);
  this->Pack(dims,6);

  double dX[3];
  b.GetGridSpacing(dX);
  this->Pack(dX,3);

  double X0[3];
  b.GetDataSetOrigin(X0);
  this->Pack(X0,3);
}

//-----------------------------------------------------------------------------
inline
void Stream::UnPack(vtkAMRBox &b)
{
  int dims[6];
  this->UnPack(dims,6);
  b.SetDimensions(dims);

  double dX[3];
  this->UnPack(dX,3);
  b.SetGridSpacing(dX);

  double X0[3];
  this->UnPack(X0,3);
  b.SetDataSetOrigin(X0);
}

//-----------------------------------------------------------------------------
template<typename T>
void Stream::Pack(vector<T> &v)
{
  const int vLen=v.size();
  this->Pack(vLen);
  this->Pack(&v[0],vLen);
}

//-----------------------------------------------------------------------------
template<typename T>
void Stream::UnPack(vector<T> &v)
{
  int vLen;
  this->UnPack(vLen);
  v.resize(vLen);
  this->UnPack(&v[0],vLen);
}

//-----------------------------------------------------------------------------
inline
void Stream::Pack(map<string,int> &m)
{
  const int mLen=m.size();
  this->Pack(mLen);

  map<string,int>::iterator it=m.begin();
  map<string,int>::iterator end=m.end();
  for (;it!=end; ++it)
    {
    this->Pack(it->first);
    this->Pack(it->second);
    }
}

//-----------------------------------------------------------------------------
inline
void Stream::UnPack(map<string,int> &m)
{
  int mLen=0;
  this->UnPack(mLen);

  for (int i=0; i<mLen; ++i)
    {
    string key;
    this->UnPack(key);
    int val;
    this->UnPack(val);

    m[key]=val;
    }
}

#endif
