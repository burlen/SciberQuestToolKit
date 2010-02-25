/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef Value_h
#define Value_h

#include "ValueUnion.h"

//=============================================================================
class Value
{
public:
  // return a new object of the same type
  virtual Value *NewInstance()=0;
  //virtual Value *Duplicate()=0;

  // reference counting machinery, derived classes should
  // implement New, and define constructors protected.
  virtual void Delete()=0;
  virtual void Register()=0;

  virtual ~Value()=0;

  // Built-in vector aware iterator for arrays of values.
  // For non-pointer values IteratorOk always fails if you
  // have to both pointer and non-pointer values then use
  // the following pattern:
  //
  // v->IteratorBegin();
  // do {
  //   ...
  //   v->IteratorAdvance();
  // } while (v->IteratorOk());
  virtual void IteratorAdvance()=0;
  virtual void IteratorBegin()=0;
  virtual bool IteratorOk()=0;

  // Logical (return the result in a new Value)
  virtual Value *Equal(Value *rhs)=0;
  virtual Value *NotEqual(Value *rhs)=0;
  virtual Value *Less(Value *rhs)=0;
  virtual Value *LessEqual(Value *rhs)=0;
  virtual Value *Greater(Value *rhs)=0;
  virtual Value *GreaterEqual(Value *rhs)=0;
  virtual Value *And(Value *rhs)=0;
  virtual Value *Or(Value *rhs)=0;
  virtual Value *Not()=0;

  // Arithmetic (return the result in a new Value)
  virtual Value *Add(Value *rhs)=0;
  virtual Value *Subtract(Value *rhs)=0;
  virtual Value *Multiply(Value *rhs)=0;
  virtual Value *Divide(Value *rhs)=0;
  virtual Value *Sqrt()=0;
  virtual Value *Negate()=0;
  // In-place arithmetic (return pointer to this)
  virtual Value *AddAssign(Value *rhs)=0;
  virtual Value *SubtractAssign(Value *rhs)=0;
  virtual Value *MultiplyAssign(Value *rhs)=0;
  virtual Value *DivideAssign(Value *rhs)=0;
  virtual Value *Assign(Value *rhs)=0;

  // Vector arirthmetic (return result in a new Value)
  virtual Value *Component(int i)=0;
  virtual Value *L2Norm()=0;

  // Type information
  virtual int GetTypeId()=0;
  virtual const char *GetTypeString()=0;

  // Print the value and type.
  virtual void Print(ostream &os)=0;

  // The value.
  ValueUnion Data;
};

Value::~Value(){}

//-----------------------------------------------------------------------------
ostream &operator<<(ostream &os,Value &v)
{
  v.Print(os);
  return os;
}

#endif
