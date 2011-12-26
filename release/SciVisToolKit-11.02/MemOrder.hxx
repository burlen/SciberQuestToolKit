/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.
*/

#include <vector>
using std::vector;

//*****************************************************************************
template<typename T>
void Split(
      size_t n,
      int nComp,
      T * __restrict__  V,
      T * __restrict__  W)
{
  // reorder a vtk vector array into three contiguous
  // scalar components
  for (size_t i=0; i<n; ++i)
    {
    size_t ii=nComp*i;
    for (int q=0; q<nComp; ++q)
      {
      size_t qq=n*q;
      W[qq+i]=V[ii+q];
    }
  }
}

//*****************************************************************************
template<typename T>
void Interleave(
      size_t n,
      int nComp,
      T * __restrict__  W,
      T * __restrict__  V)
{
  // take an irray that has been ordered contiguously by component
  // and interleave
  for (size_t i=0; i<n; ++i)
    {
    size_t ii=nComp*i;
    for (int q=0; q<nComp; ++q)
      {
      size_t qq=n*q;
      V[ii+q]=W[qq+i];
    }
  }
}

//*****************************************************************************
template<typename T>
void Split(
      size_t n,
      T * __restrict__ V,
      vector<T*> &W)
{
  // reorder a vtk vector array into contiguous
  // scalar components
  int nComp=W.size();
  for (size_t i=0; i<n; ++i)
    {
    size_t ii=nComp*i;
    for (int q=0; q<nComp; ++q)
      {
      W[q][i]=V[ii+q];
      }
    }
}

//*****************************************************************************
template<typename T>
void Interleave(
      size_t n,
      vector<T *> &W,
      T * __restrict__  V)
{
  // interleave array in contiguous component order
  int nComp=W.size();
  for (size_t i=0; i<n; ++i)
    {
    size_t ii=nComp*i;
    for (int q=0; q<nComp; ++q)
      {
      V[ii+q]=W[q][i];
    }
  }
}

