/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef VariantTraits_h
#define VariantTraits_h

//=============================================================================
typedef
  enum {
    CHAR_TYPE,
    CHAR_PTR_TYPE,
    BOOL_TYPE,
    BOOL_PTR_TYPE,
    INT_TYPE,
    INT_PTR_TYPE,
    FLOAT_TYPE,
    FLOAT_PTR_TYPE,
    DOUBLE_TYPE,
    DOUBLE_PTR_TYPE,
    LONGLONG_TYPE,
    LONGLONG_PTR_TYPE
    }
VariantTypeIdentifier;

//=============================================================================
template <typename T>
class VariantTraits {};

// Two types of scpecializations, one for values and one for pointers to
// values. The latter need to be dereferenced during the access and can 
// be advanced.


#define VariantTraitsSpecialization(ID,TYPE,UTYPE)\
template<>\
class VariantTraits<TYPE>\
{\
public:\
  VariantTraits(){}\
  typedef TYPE VariantType;\
  int TypeId(){ return ID; }\
  const char *TypeString(){ return #TYPE; }\
  TYPE &Access(VariantUnion &vu, int ofs=0){ return vu.UTYPE; }\
  void Assign(VariantUnion &vu, TYPE val){ vu.UTYPE=val; }\
  void InitializeIterator(int nComps, int nTups){}\
  void IteratorAdvance(){}\
  void IteratorBegin(){}\
  bool IteratorOk(){ return false; }\
  int GetNComps(){ return 1; }\
};

#define VariantTraitsPtrSpecialization(ID,TYPE,UTYPE)\
template<>\
class VariantTraits<TYPE *>\
{\
public:\
  VariantTraits()\
      :\
    NComps(1),\
    End(0),\
    At(0)\
    {}\
  typedef TYPE VariantType;\
  int TypeId(){ return ID; }\
  const char *TypeString(){ return #UTYPE; }\
  TYPE &Access(const VariantUnion &vu){ return vu.UTYPE[this->At]; }\
  TYPE &Access(const VariantUnion &vu, int ofs){ return vu.UTYPE[this->At+ofs]; }\
  void Assign(VariantUnion &vu, TYPE *val){ vu.UTYPE=val; }\
  void Assign(VariantUnion &vu, TYPE val){ vu.UTYPE[this->At]=val; }\
  void InitializeIterator(int nComps, int nTups){\
    this->NComps=nComps;\
    this->End=nComps*nTups;\
    }\
  void IteratorAdvance(){ this->At+=this->NComps; }\
  void IteratorBegin(){ this->At=0; }\
  bool IteratorOk(){ return this->At<this->End; }\
  int GetNComps(){ return this->NComps; }\
private:\
  int NComps;\
  int End;\
  int At;\
};

VariantTraitsSpecialization(CHAR_TYPE,char,Char);
VariantTraitsSpecialization(BOOL_TYPE,bool,Bool);
VariantTraitsSpecialization(INT_TYPE,int,Int);
VariantTraitsSpecialization(FLOAT_TYPE,float,Float);
VariantTraitsSpecialization(DOUBLE_TYPE,double,Double);
VariantTraitsSpecialization(LONGLONG_TYPE,long long,LongLong);

VariantTraitsPtrSpecialization(CHAR_PTR_TYPE,char,CharPtr);
VariantTraitsPtrSpecialization(BOOL_PTR_TYPE,bool,BoolPtr);
VariantTraitsPtrSpecialization(INT_PTR_TYPE,int,IntPtr);
VariantTraitsPtrSpecialization(FLOAT_PTR_TYPE,float,FloatPtr);
VariantTraitsPtrSpecialization(DOUBLE_PTR_TYPE,double,DoublePtr);
VariantTraitsPtrSpecialization(LONGLONG_PTR_TYPE,long long,LongLongPtr);

#endif
