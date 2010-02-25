/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef ValueTraits_h
#define ValueTraits_h

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
ValueTypeIdentifier;

//=============================================================================
template <typename T>
class ValueTraits {};

// Two types of scpecializations, one for values and one for pointers to
// values. The latter need to be dereferenced during the access and can 
// be advanced.


#define ValueTraitsSpecialization(ID,TYPE,UTYPE)\
template<>\
class ValueTraits<TYPE>\
{\
public:\
  ValueTraits(){}\
  typedef TYPE ValueType;\
  int TypeId(){ return ID; }\
  const char *TypeString(){ return #TYPE; }\
  TYPE &Access(ValueUnion &vu, int ofs=0){ return vu.UTYPE; }\
  void Assign(ValueUnion &vu, TYPE val){ vu.UTYPE=val; }\
  void InitializeIterator(int nComps, int nTups){}\
  void IteratorAdvance(){}\
  void IteratorBegin(){}\
  bool IteratorOk(){ return false; }\
  int GetNComps(){ return 1; }\
};

#define ValueTraitsPtrSpecialization(ID,TYPE,UTYPE)\
template<>\
class ValueTraits<TYPE *>\
{\
public:\
  ValueTraits()\
      :\
    NComps(1),\
    End(0),\
    At(0)\
    {}\
  typedef TYPE ValueType;\
  int TypeId(){ return ID; }\
  const char *TypeString(){ return #UTYPE; }\
  TYPE &Access(const ValueUnion &vu){ return vu.UTYPE[this->At]; }\
  TYPE &Access(const ValueUnion &vu, int ofs){ return vu.UTYPE[this->At+ofs]; }\
  void Assign(ValueUnion &vu, TYPE *val){ vu.UTYPE=val; }\
  void Assign(ValueUnion &vu, TYPE val){ vu.UTYPE[this->At]=val; }\
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

ValueTraitsSpecialization(CHAR_TYPE,char,Char);
ValueTraitsSpecialization(BOOL_TYPE,bool,Bool);
ValueTraitsSpecialization(INT_TYPE,int,Int);
ValueTraitsSpecialization(FLOAT_TYPE,float,Float);
ValueTraitsSpecialization(DOUBLE_TYPE,double,Double);
ValueTraitsSpecialization(LONGLONG_TYPE,long long,LongLong);

ValueTraitsPtrSpecialization(CHAR_PTR_TYPE,char,CharPtr);
ValueTraitsPtrSpecialization(BOOL_PTR_TYPE,bool,BoolPtr);
ValueTraitsPtrSpecialization(INT_PTR_TYPE,int,IntPtr);
ValueTraitsPtrSpecialization(FLOAT_PTR_TYPE,float,FloatPtr);
ValueTraitsPtrSpecialization(DOUBLE_PTR_TYPE,double,DoublePtr);
ValueTraitsPtrSpecialization(LONGLONG_PTR_TYPE,long long,LongLongPtr);

#endif
