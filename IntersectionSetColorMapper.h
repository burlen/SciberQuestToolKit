/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef IntersectionSetColorMapper_h
#define IntersectionSetColorMapper_h

#include "IntersectionSet.h"

#include <vector>
using std::vector;

namespace {
//*****************************************************************************
template<typename T>
T max(T a, T b){ return a<b?b:a; }

//*****************************************************************************
template<typename T>
T min(T a, T b){ return a<b?a:b; }
};

/**
/// Class that manages color assignement operation for insterection sets.
Class that manages color assignement operation for insterection sets. This class
assigns an intersection based on the intersection set surface ids and the number of
surfaces. Two sets, S1 and S2, are equivalent (and tehrefor are assigned the same
color) when S1=(a b) and S2=(b a).
*/
class IntersectionSetColorMapper
{
public:
  /// Construct in an invalid state.
  IntersectionSetColorMapper() : NSurfaces(0) {};
  /// Copy construct.
  IntersectionSetColorMapper(const IntersectionSetColorMapper &other){
    *this=other;
    }
  /// Assignment.
  const IntersectionSetColorMapper &operator=(const IntersectionSetColorMapper &other){
    if (this!=&other)
      {
      this->NSurfaces=other.NSurfaces;
      this->Colors=other.Colors;
      }
    return *this;
    }
  /// Set the number of surfaces, this allocates space for the colors.
  /// and appropriately initializes them.
  void SetNumberOfSurfaces(int nSurfaces){
    // Store the color values in the upper triangle of a nxn matrix.
    // Note that we only need n=sum_{i=1}^{n+1}i colors, but using the
    // matrix sinmplifies look ups.
    this->NSurfaces=nSurfaces;
    const int nColors=(nSurfaces+1)*(nSurfaces+1);
    this->Colors.clear();
    this->Colors.resize(nColors,-1);
    int color=0;
    for (int j=0; j<nSurfaces+1; ++j)
      {
      for (int i=j; i<nSurfaces+1; ++i)
        {
        int x=max(i,j);
        int y=min(i,j);
        int idx=x+(nSurfaces+1)*y;
        this->Colors[idx]=color;
        ++color;
        }
      }
    }
  /// Get color associated with an intersection set.
  int LookupColor(const IntersectionSet &iset){
    int s1=iset.GetForwardIntersectionId()+1;
    int s2=iset.GetBackwardIntersectionId()+1;
    return this->LookupColor(s1,s2);
    }
  /// Get color associated with 2 surfaces.
  int LookupColor(const int s1, const int s2){
    int x=max(s1,s2);
    int y=min(s1,s2);
    int idx=x+(this->NSurfaces+1)*y;
    return this->Colors[idx];
    }

private:
  int NSurfaces;
  vector<int> Colors;
};

#endif
