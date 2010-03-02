/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef VariantTemplate_h
#define VariantTemplate_h

#include <cmath>

#include "Variant.h"
#include "VariantTraits.h"

#define VariantTemplateUnaryOperatorDecl(RET_TYPE,METHOD_NAME)\
/* Apply the operator to this. */\
VariantTemplate<RET_TYPE> *METHOD_NAME();\

/* Dispatch the correct templated operator based on this and rhs type. 
virtual Variant *METHOD_NAME() const;*/

#define VariantTemplateBinaryOperatorDecl(RET_TYPE,METHOD_NAME)\
/* Apply the operator to this. */\
template<typename S>\
VariantTemplate<RET_TYPE> *METHOD_NAME(VariantTemplate<S> *rhs);\
\
/* Dispatch the correct templated operator based on this and rhs type. */\
virtual Variant *METHOD_NAME(Variant *rhs);

//=============================================================================
template<typename T>
class VariantTemplate : public Variant
{
public:
  // Refcounted pointer machinery
  static VariantTemplate<T> *New(T data);
  static VariantTemplate<T> *New(T data,int nTups);
  static VariantTemplate<T> *New(T data,int nComp,int nTups);

  virtual ~VariantTemplate();

  virtual void Print(ostream &os);

  virtual int GetTypeId();
  virtual const char *GetTypeString();

  typedef typename VariantTraits<T>::VariantType VariantType;

  virtual Variant *NewInstance(){ return VariantTemplate<VariantType>::New(0); }

  // Description:
  // Access to the object's value by reference ie Set/Get
  virtual  VariantType &GetValue() { return this->Traits.Access(this->Data); }
  virtual  VariantType &GetValue(int i) { return this->Traits.Access(this->Data,i); }

  // Description:
  // Initialize the object's value and number of components.
  virtual void Initialize(T data) { this->Initialize(data,1,1); }
  virtual void Initialize(T data,int nComps,int nTups);

  // Description:
  // Built in vector aware iterator for arrays of values.
  virtual void IteratorAdvance() { this->Traits.IteratorAdvance(); }
  virtual void IteratorBegin() { this->Traits.IteratorBegin(); }
  virtual bool IteratorOk() { return this->Traits.IteratorOk(); }

  // Logical operators - returns a new VariantTemplate<bool>
  VariantTemplateBinaryOperatorDecl(bool,Equal)
  VariantTemplateBinaryOperatorDecl(bool,NotEqual)
  VariantTemplateBinaryOperatorDecl(bool,Less)
  VariantTemplateBinaryOperatorDecl(bool,LessEqual)
  VariantTemplateBinaryOperatorDecl(bool,Greater)
  VariantTemplateBinaryOperatorDecl(bool,GreaterEqual)
  VariantTemplateBinaryOperatorDecl(bool,And)
  VariantTemplateBinaryOperatorDecl(bool,Or)
  VariantTemplateUnaryOperatorDecl(bool,Not)

  // Arithmetic operators - returns a new VariantTemplate<double>
  VariantTemplateBinaryOperatorDecl(double,Add)
  VariantTemplateBinaryOperatorDecl(double,Subtract)
  VariantTemplateBinaryOperatorDecl(double,Multiply)
  VariantTemplateBinaryOperatorDecl(double,Divide)
  VariantTemplateUnaryOperatorDecl(double,Sqrt)
  VariantTemplateUnaryOperatorDecl(double,Negate)
  // In-place arithmetic operators - returns a new VariantTemplate<T>
  VariantTemplateBinaryOperatorDecl(T,AddAssign)
  VariantTemplateBinaryOperatorDecl(T,SubtractAssign)
  VariantTemplateBinaryOperatorDecl(T,MultiplyAssign)
  VariantTemplateBinaryOperatorDecl(T,DivideAssign)
  VariantTemplateBinaryOperatorDecl(T,Assign)

  // Vector arithmetic - return a new VariantTemplate<double>
  virtual VariantTemplate<VariantType> *Component(int i);
  virtual VariantTemplate<double> *L2Norm();

protected:
  VariantTemplate(T data);
  VariantTemplate(T data,int nComp,int nTups);

private:
  VariantTemplate(); // not implemented
  VariantTemplate(const VariantTemplate &);
  VariantTemplate &operator=(const VariantTemplate &);

private:
  VariantTraits<T> Traits;
};

//-----------------------------------------------------------------------------
template<typename T>
VariantTemplate<T> *VariantTemplate<T>::New(T data)
{
  return new VariantTemplate<T>(data);
}

//-----------------------------------------------------------------------------
template<typename T>
VariantTemplate<T> *VariantTemplate<T>::New(T data,int nTups)
{
  return new VariantTemplate<T>(data,1,nTups);
}

//-----------------------------------------------------------------------------
template<typename T>
VariantTemplate<T> *VariantTemplate<T>::New(T data,int nComp,int nTups)
{
  return new VariantTemplate<T>(data,nComp,nTups);
}

//-----------------------------------------------------------------------------
template<typename T>
VariantTemplate<T>::VariantTemplate(T data)
{
  this->Initialize(data);
}

//-----------------------------------------------------------------------------
template<typename T>
VariantTemplate<T>::VariantTemplate(T data, int nComps, int nTups)
{
  //this->RefCount=1;
  this->Initialize(data,nComps,nTups);
}

//-----------------------------------------------------------------------------
template<typename T>
VariantTemplate<T>::~VariantTemplate()
{}

//-----------------------------------------------------------------------------
template<typename T>
void VariantTemplate<T>::Initialize(T data, int nComps, int nTups)
{
  this->Traits.Assign(this->Data,data);
  this->Traits.InitializeIterator(nComps,nTups);
}

//-----------------------------------------------------------------------------
template<typename T>
VariantTemplate<typename VariantTraits<T>::VariantType> *VariantTemplate<T>::Component(int i)
{
  typedef typename VariantTraits<T>::VariantType VariantType;
  return VariantTemplate<VariantType>::New(this->Traits.Access(this->Data,i));
}

//-----------------------------------------------------------------------------
template<typename T>
VariantTemplate<double> *VariantTemplate<T>::L2Norm()
{
  double norm=0.0;
  int nComps=this->Traits.GetNComps();
  for (int i=0; i<nComps; ++i)
    {
    double v=this->Traits.Access(this->Data,i);
    norm+=v*v;
    }
  norm=sqrt(norm);
  return VariantTemplate<double>::New(norm);
}

//-----------------------------------------------------------------------------
template<typename T>
void VariantTemplate<T>::Print(ostream &os)
{
  //this->RefCountedPointer::Print(os); cerr << " ";

  os << this->GetValue();
  int nComps=this->Traits.GetNComps();
  for (int i=1; i<nComps; ++i)
    {
    os << ", " << this->GetValue(i);
    }
}

//-----------------------------------------------------------------------------
template<typename T>
int VariantTemplate<T>::GetTypeId()
{
  return this->Traits.TypeId();
}

//-----------------------------------------------------------------------------
template<typename T>
const char *VariantTemplate<T>::GetTypeString()
{
  return this->Traits.TypeString();
}


#define VariantTemplateUnaryOperatorImpl(RET_TYPE,METHOD_NAME,CPP_OP)\
/*-----------------------------------------------------------------------------*/\
template<typename T>\
VariantTemplate<RET_TYPE> *VariantTemplate<T>::METHOD_NAME()\
{\
  return VariantTemplate<RET_TYPE>::New(CPP_OP(this->GetValue()));\
}

#define VariantTemplateBinaryOperatorDispatchImpl(RET_TYPE,METHOD_NAME,CPP_OP)\
/*-----------------------------------------------------------------------------*/\
template<typename T>\
Variant *VariantTemplate<T>::METHOD_NAME(Variant *rhs)\
{\
  /* Dsipatch the right method. LHS type is handled by polymorphism\
     RHS must be explicitly cast. */\
  switch (rhs->GetTypeId())\
    {\
    case CHAR_TYPE:\
      return this->METHOD_NAME((VariantTemplate<char>*)rhs);\
      break;\
    case CHAR_PTR_TYPE:\
      return this->METHOD_NAME((VariantTemplate<char*>*)rhs);\
      break;\
    case BOOL_TYPE:\
      return this->METHOD_NAME((VariantTemplate<bool>*)rhs);\
      break;\
    case BOOL_PTR_TYPE:\
      return this->METHOD_NAME((VariantTemplate<bool*>*)rhs);\
      break;\
    case INT_TYPE:\
      return this->METHOD_NAME((VariantTemplate<int>*)rhs);\
      break;\
    case INT_PTR_TYPE:\
      return this->METHOD_NAME((VariantTemplate<int*>*)rhs);\
      break;\
    case FLOAT_TYPE:\
      return this->METHOD_NAME((VariantTemplate<float>*)rhs);\
      break;\
    case FLOAT_PTR_TYPE:\
      return this->METHOD_NAME((VariantTemplate<float*>*)rhs);\
      break;\
    case DOUBLE_TYPE:\
      return this->METHOD_NAME((VariantTemplate<double>*)rhs);\
      break;\
    case DOUBLE_PTR_TYPE:\
      return this->METHOD_NAME((VariantTemplate<double*>*)rhs);\
      break;\
    case LONGLONG_TYPE:\
      return this->METHOD_NAME((VariantTemplate<long long>*)rhs);\
      break;\
    case LONGLONG_PTR_TYPE:\
      return this->METHOD_NAME((VariantTemplate<long long*>*)rhs);\
      break;\
    }\
  /* Execution never reaches this when used correctly. */\
  return 0;\
}

#define VariantTemplateBinaryOperatorImpl(RET_TYPE,METHOD_NAME,CPP_OP)\
/*-----------------------------------------------------------------------------*/\
template<typename T>\
template<typename S>\
VariantTemplate<RET_TYPE> *VariantTemplate<T>::METHOD_NAME(VariantTemplate<S> *rhs)\
{\
  return VariantTemplate<RET_TYPE>::New(this->GetValue() CPP_OP rhs->GetValue());\
}\
\
VariantTemplateBinaryOperatorDispatchImpl(RET_TYPE,METHOD_NAME,CPP_OP)

#define VariantTemplateInPlaceBinaryOperatorImpl(METHOD_NAME,CPP_OP)\
/*-----------------------------------------------------------------------------*/\
template<typename T>\
template<typename S>\
VariantTemplate<T> *VariantTemplate<T>::METHOD_NAME(VariantTemplate<S> *rhs)\
{\
  this->GetValue() CPP_OP rhs->GetValue();\
  return this;\
}\
\
VariantTemplateBinaryOperatorDispatchImpl(T,METHOD_NAME,CPP_OP)

// Logical
VariantTemplateBinaryOperatorImpl(bool,Equal,==)
VariantTemplateBinaryOperatorImpl(bool,NotEqual,!=)
VariantTemplateBinaryOperatorImpl(bool,Less,<)
VariantTemplateBinaryOperatorImpl(bool,LessEqual,<=)
VariantTemplateBinaryOperatorImpl(bool,Greater,>)
VariantTemplateBinaryOperatorImpl(bool,GreaterEqual,>=)
VariantTemplateBinaryOperatorImpl(bool,And,&&)
VariantTemplateBinaryOperatorImpl(bool,Or,||)
VariantTemplateUnaryOperatorImpl(bool,Not,!)

// Arithmetic
VariantTemplateBinaryOperatorImpl(double,Add,+)
VariantTemplateBinaryOperatorImpl(double,Subtract,-)
VariantTemplateBinaryOperatorImpl(double,Multiply,*)
VariantTemplateBinaryOperatorImpl(double,Divide,/)
VariantTemplateUnaryOperatorImpl(double,Sqrt,sqrt)
VariantTemplateUnaryOperatorImpl(double,Negate,-)
// In-place arithmetic
VariantTemplateInPlaceBinaryOperatorImpl(AddAssign,+=)
VariantTemplateInPlaceBinaryOperatorImpl(SubtractAssign,-=)
VariantTemplateInPlaceBinaryOperatorImpl(MultiplyAssign,*=)
VariantTemplateInPlaceBinaryOperatorImpl(DivideAssign,/=)
VariantTemplateInPlaceBinaryOperatorImpl(Assign,=)

#endif
