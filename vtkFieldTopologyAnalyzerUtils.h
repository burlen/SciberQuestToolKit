#ifndef vtkTopologyAnalyzerUtil_h
#define vtkTopologyAnalyzerUtil_h

#include "mpi.h"
#include <string>
using std::string;
#include <vector>
using std::vector;

//*****************************************************************************
template<typename T>
T max(T a, T b){ return a<b?b:a; }

//*****************************************************************************
template<typename T>
T min(T a, T b){ return a<b?a:b; }

//*****************************************************************************
inline bool InRange(int a, int b, int v){ return v>=a && v<=b; }

//*****************************************************************************
inline bool ValidId(int id){ return id>=0; }

//*****************************************************************************
inline int InvalidId(){ return -1; }


/**
/// Data associated with a stream line surface intersection set.
Data associated with a stream line surface intersection set. Streamlines 
are identified by their seedpoint ids. Each streamline is streated internally
in two parts a forward running part and a backward running part, each of which
may have an intersection. Nothing says that streamline may not intersect 
multiple surfaces multiple times. We use the integration time as the parameter
to decide which surface is closest to the seed point and discard others.
*/
class IntersectData
{
public:
  /// Construct in an invalid state.
  IntersectData()
    :
  seedPointId(-1),
  fwdSurfaceId(-1),
  bwdSurfaceId(-1),
  fwdIntersectTime(-1.0),
  bwdIntersectTime(-1.0)
    {}

  /// Construct in an initialized state.
  IntersectData(
    int seed, int fwdSurface, double fwdTime, int bwdSurface, double bwdTime)
    :
  seedPointId(seed),
  fwdSurfaceId(fwdSurface),
  bwdSurfaceId(bwdSurface),
  fwdIntersectTime(fwdTime),
  bwdIntersectTime(bwdTime)
    {}

  /// Copy construct
  IntersectData(const IntersectData &other);

  /// Assignment
  const IntersectData &operator=(const IntersectData &other);

  /// Create the MPI compatible description of this.
  int CommitType(MPI_Datatype *classType);

  /// Print the object state to a string.
  string Print();

public:
  int seedPointId;
  int fwdSurfaceId;
  int bwdSurfaceId;
  double fwdIntersectTime;
  double bwdIntersectTime;
};

/**
/// Container for data associated with a streamline surface intersection set.
Provides an interface to operate on the data associated with the intersection
set of a streamline (uniquely associated with a seed point).
*/
class IntersectionSet
{
public:
  /// Construct with id's marked invlaid.
  IntersectionSet();
  ~IntersectionSet();

  /// Copy construct
  IntersectionSet(const IntersectionSet &other);

  /// Assignment
  const IntersectionSet &operator=(const IntersectionSet &other);

  /// Set/Get seed point ids.
  void SetSeedPointId(int id){
    this->Data.seedPointId=id;
    }
  int GetSeedPointId(){
    return this->Data.seedPointId;
    }

  /// Set/Get intersection set values.
  void SetForwardIntersection(int surfaceId, double time){
    this->Data.fwdSurfaceId=surfaceId;
    this->Data.fwdIntersectTime=time;
    }
  int GetForwardIntersectionId() const {
    return this->Data.fwdSurfaceId;
    }
  void SetBackwardIntersection(int surfaceId, double time){
    this->Data.bwdSurfaceId=surfaceId;
    this->Data.bwdIntersectTime=time;
    }
  int GetBackwardIntersectionId() const {
    return this->Data.bwdSurfaceId;
    }

  /// Insert an intersection set only if its closer than the current.
  void InsertForwardIntersection(int surfaceId, double time){
    IntersectData other(-1,surfaceId,time,-1,-1.0);
    this->Reduce(other);
    }
  void InsertBackwardIntersection(int surfaceId, double time){
    IntersectData other(-1,-1,-1.0,surfaceId,time);
    this->Reduce(other);
    }

  /// Assign color based on the intersecting surface ids, and
  /// the number of surfaces. The value returned is a coordinate
  /// in a 2-D plane with 0 to nSurfaces values on each axis.
  int Color(int nSurfaces){
    int x=this->Data.fwdSurfaceId+1;
    int y=this->Data.bwdSurfaceId+1;
    return x+(nSurfaces+1)*y;
    }

  /// Synchronize intersection sets across all processes, such that 
  /// all reflect the intersection set closest to this seed point.
  int AllReduce();

  /// Print object state into a string.
  string Print();

private:
  /// Reduce in place.
  void Reduce(IntersectData &other);

private:
  IntersectData Data;     // describes surface intersections for a seed
  MPI_Datatype DataType;  // something MPI understands
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
