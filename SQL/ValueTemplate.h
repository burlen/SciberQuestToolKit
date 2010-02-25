/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef ValueTemplate_h
#define ValueTemplate_h

#include <cmath>

#include "Value.h"
#include "ValueTraits.h"

#define ValueTemplateUnaryOperatorDecl(RET_TYPE,METHOD_NAME)\
/* Apply the operator to this. */\
ValueTemplate<RET_TYPE> *METHOD_NAME();\

/* Dispatch the correct templated operator based on this and rhs type. 
virtual Value *METHOD_NAME() const;*/

#define ValueTemplateBinaryOperatorDecl(RET_TYPE,METHOD_NAME)\
/* Apply the operator to this. */\
template<typename S>\
ValueTemplate<RET_TYPE> *METHOD_NAME(ValueTemplate<S> *rhs);\
\
/* Dispatch the correct templated operator based on this and rhs type. */\
virtual Value *METHOD_NAME(Value *rhs);

//=============================================================================
template<typename T>
class ValueTemplate : public Value
{
public:
  // Refcounted pointer machinery
  static ValueTemplate<T> *New(T data);
  static ValueTemplate<T> *New(T data,int nTups);
  static ValueTemplate<T> *New(T data,int nComp,int nTups);
  virtual void Delete();
  virtual void Register(){ ++this->RefCount; }

  virtual ~ValueTemplate();

  virtual void Print(ostream &os);

  virtual int GetTypeId();
  virtual const char *GetTypeString();

  typedef typename ValueTraits<T>::ValueType ValueType;

  virtual Value *NewInstance(){ return ValueTemplate<ValueType>::New(0); }

  // Description:
  // Access to the object's value by reference ie Set/Get
  virtual  ValueType &GetValue() { return this->Traits.Access(this->Data); }
  virtual  ValueType &GetValue(int i) { return this->Traits.Access(this->Data,i); }

  // Description:
  // Initialize the object's value and number of components.
  virtual void Initialize(T data) { this->Initialize(data,1,1); }
  virtual void Initialize(T data,int nComps,int nTups);

  // Description:
  // Built in vector aware iterator for arrays of values.
  virtual void IteratorAdvance() { this->Traits.IteratorAdvance(); }
  virtual void IteratorBegin() { this->Traits.IteratorBegin(); }
  virtual bool IteratorOk() { return this->Traits.IteratorOk(); }

  // Logical operators - returns a new ValueTemplate<bool>
  ValueTemplateBinaryOperatorDecl(bool,Equal)
  ValueTemplateBinaryOperatorDecl(bool,NotEqual)
  ValueTemplateBinaryOperatorDecl(bool,Less)
  ValueTemplateBinaryOperatorDecl(bool,LessEqual)
  ValueTemplateBinaryOperatorDecl(bool,Greater)
  ValueTemplateBinaryOperatorDecl(bool,GreaterEqual)
  ValueTemplateBinaryOperatorDecl(bool,And)
  ValueTemplateBinaryOperatorDecl(bool,Or)
  ValueTemplateUnaryOperatorDecl(bool,Not)

  // Arithmetic operators - returns a new ValueTemplate<double>
  ValueTemplateBinaryOperatorDecl(double,Add)
  ValueTemplateBinaryOperatorDecl(double,Subtract)
  ValueTemplateBinaryOperatorDecl(double,Multiply)
  ValueTemplateBinaryOperatorDecl(double,Divide)
  ValueTemplateUnaryOperatorDecl(double,Sqrt)
  ValueTemplateUnaryOperatorDecl(double,Negate)
  // In-place arithmetic operators - returns a new ValueTemplate<T>
  ValueTemplateBinaryOperatorDecl(T,AddAssign)
  ValueTemplateBinaryOperatorDecl(T,SubtractAssign)
  ValueTemplateBinaryOperatorDecl(T,MultiplyAssign)
  ValueTemplateBinaryOperatorDecl(T,DivideAssign)
  ValueTemplateBinaryOperatorDecl(T,Assign)

  // Vector arithmetic - return a new ValueTemplate<double>
  virtual ValueTemplate<ValueType> *Component(int i);
  virtual ValueTemplate<double> *L2Norm();

protected:
  ValueTemplate(T data);
  ValueTemplate(T data,int nComp,int nTups);

private:
  ValueTemplate(); // not implemented

private:
  ValueTraits<T> Traits;
  int RefCount;
};

//-----------------------------------------------------------------------------
template<typename T>
ValueTemplate<T> *ValueTemplate<T>::New(T data)
{
  return new ValueTemplate<T>(data);
}

//-----------------------------------------------------------------------------
template<typename T>
ValueTemplate<T> *ValueTemplate<T>::New(T data,int nTups)
{
  return new ValueTemplate<T>(data,1,nTups);
}

//-----------------------------------------------------------------------------
template<typename T>
ValueTemplate<T> *ValueTemplate<T>::New(T data,int nComp,int nTups)
{
  return new ValueTemplate<T>(data,nComp,nTups);
}

//-----------------------------------------------------------------------------
template<typename T>
ValueTemplate<T>::ValueTemplate(T data)
{
  this->RefCount=1;
  this->Initialize(data);
}

//-----------------------------------------------------------------------------
template<typename T>
void ValueTemplate<T>::Delete()
{
  --this->RefCount;
  if (this->RefCount<=0)
    {
    delete this;
    }
}

//-----------------------------------------------------------------------------
template<typename T>
ValueTemplate<T>::ValueTemplate(T data, int nComps, int nTups)
{
  this->RefCount=1;
  this->Initialize(data,nComps,nTups);
}

//-----------------------------------------------------------------------------
template<typename T>
ValueTemplate<T>::~ValueTemplate()
{}

//-----------------------------------------------------------------------------
template<typename T>
void ValueTemplate<T>::Initialize(T data, int nComps, int nTups)
{
  this->Traits.Assign(this->Data,data);
  this->Traits.InitializeIterator(nComps,nTups);
}

//-----------------------------------------------------------------------------
template<typename T>
ValueTemplate<typename ValueTraits<T>::ValueType> *ValueTemplate<T>::Component(int i)
{
  typedef typename ValueTraits<T>::ValueType ValueType;
  return ValueTemplate<ValueType>::New(this->Traits.Access(this->Data,i));
}

//-----------------------------------------------------------------------------
template<typename T>
ValueTemplate<double> *ValueTemplate<T>::L2Norm()
{
  double norm=0.0;
  int nComps=this->Traits.GetNComps();
  for (int i=0; i<nComps; ++i)
    {
    double v=this->Traits.Access(this->Data,i);
    norm+=v*v;
    }
  norm=sqrt(norm);
  return ValueTemplate<double>::New(norm);
}

//-----------------------------------------------------------------------------
template<typename T>
void ValueTemplate<T>::Print(ostream &os)
{
  os << this->GetValue();
  int nComps=this->Traits.GetNComps();
  for (int i=1; i<nComps; ++i)
    {
    os << ", " << this->GetValue(i);
    }
}

//-----------------------------------------------------------------------------
template<typename T>
int ValueTemplate<T>::GetTypeId()
{
  return this->Traits.TypeId();
}

//-----------------------------------------------------------------------------
template<typename T>
const char *ValueTemplate<T>::GetTypeString()
{
  return this->Traits.TypeString();
}


#define ValueTemplateUnaryOperatorImpl(RET_TYPE,METHOD_NAME,CPP_OP)\
/*-----------------------------------------------------------------------------*/\
template<typename T>\
ValueTemplate<RET_TYPE> *ValueTemplate<T>::METHOD_NAME()\
{\
  return ValueTemplate<RET_TYPE>::New(CPP_OP(this->GetValue()));\
}

#define ValueTemplateBinaryOperatorDispatchImpl(RET_TYPE,METHOD_NAME,CPP_OP)\
/*-----------------------------------------------------------------------------*/\
template<typename T>\
Value *ValueTemplate<T>::METHOD_NAME(Value *rhs)\
{\
  /* Dsipatch the right method. LHS type is handled by polymorphism\
     RHS must be explicitly cast. */\
  switch (rhs->GetTypeId())\
    {\
    case CHAR_TYPE:\
      return this->METHOD_NAME((ValueTemplate<char>*)rhs);\
      break;\
    case CHAR_PTR_TYPE:\
      return this->METHOD_NAME((ValueTemplate<char*>*)rhs);\
      break;\
    case BOOL_TYPE:\
      return this->METHOD_NAME((ValueTemplate<bool>*)rhs);\
      break;\
    case BOOL_PTR_TYPE:\
      return this->METHOD_NAME((ValueTemplate<bool*>*)rhs);\
      break;\
    case INT_TYPE:\
      return this->METHOD_NAME((ValueTemplate<int>*)rhs);\
      break;\
    case INT_PTR_TYPE:\
      return this->METHOD_NAME((ValueTemplate<int*>*)rhs);\
      break;\
    case FLOAT_TYPE:\
      return this->METHOD_NAME((ValueTemplate<float>*)rhs);\
      break;\
    case FLOAT_PTR_TYPE:\
      return this->METHOD_NAME((ValueTemplate<float*>*)rhs);\
      break;\
    case DOUBLE_TYPE:\
      return this->METHOD_NAME((ValueTemplate<double>*)rhs);\
      break;\
    case DOUBLE_PTR_TYPE:\
      return this->METHOD_NAME((ValueTemplate<double*>*)rhs);\
      break;\
    case LONGLONG_TYPE:\
      return this->METHOD_NAME((ValueTemplate<long long>*)rhs);\
      break;\
    case LONGLONG_PTR_TYPE:\
      return this->METHOD_NAME((ValueTemplate<long long*>*)rhs);\
      break;\
    }\
  /* Execution never reaches this when used correctly. */\
  return 0;\
}

#define ValueTemplateBinaryOperatorImpl(RET_TYPE,METHOD_NAME,CPP_OP)\
/*-----------------------------------------------------------------------------*/\
template<typename T>\
template<typename S>\
ValueTemplate<RET_TYPE> *ValueTemplate<T>::METHOD_NAME(ValueTemplate<S> *rhs)\
{\
  return ValueTemplate<RET_TYPE>::New(this->GetValue() CPP_OP rhs->GetValue());\
}\
\
ValueTemplateBinaryOperatorDispatchImpl(RET_TYPE,METHOD_NAME,CPP_OP)

#define ValueTemplateInPlaceBinaryOperatorImpl(METHOD_NAME,CPP_OP)\
/*-----------------------------------------------------------------------------*/\
template<typename T>\
template<typename S>\
ValueTemplate<T> *ValueTemplate<T>::METHOD_NAME(ValueTemplate<S> *rhs)\
{\
  this->GetValue() CPP_OP rhs->GetValue();\
  return this;\
}\
\
ValueTemplateBinaryOperatorDispatchImpl(T,METHOD_NAME,CPP_OP)

// Logical
ValueTemplateBinaryOperatorImpl(bool,Equal,==)
ValueTemplateBinaryOperatorImpl(bool,NotEqual,!=)
ValueTemplateBinaryOperatorImpl(bool,Less,<)
ValueTemplateBinaryOperatorImpl(bool,LessEqual,<=)
ValueTemplateBinaryOperatorImpl(bool,Greater,>)
ValueTemplateBinaryOperatorImpl(bool,GreaterEqual,>=)
ValueTemplateBinaryOperatorImpl(bool,And,&&)
ValueTemplateBinaryOperatorImpl(bool,Or,||)
ValueTemplateUnaryOperatorImpl(bool,Not,!)

// Arithmetic
ValueTemplateBinaryOperatorImpl(double,Add,+)
ValueTemplateBinaryOperatorImpl(double,Subtract,-)
ValueTemplateBinaryOperatorImpl(double,Multiply,*)
ValueTemplateBinaryOperatorImpl(double,Divide,/)
ValueTemplateUnaryOperatorImpl(double,Sqrt,sqrt)
ValueTemplateUnaryOperatorImpl(double,Negate,-)
// In-place arithmetic
ValueTemplateInPlaceBinaryOperatorImpl(AddAssign,+=)
ValueTemplateInPlaceBinaryOperatorImpl(SubtractAssign,-=)
ValueTemplateInPlaceBinaryOperatorImpl(MultiplyAssign,*=)
ValueTemplateInPlaceBinaryOperatorImpl(DivideAssign,/=)
ValueTemplateInPlaceBinaryOperatorImpl(Assign,=)

#endif
