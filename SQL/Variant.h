/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef Variant_h
#define Variant_h

#include <iostream>
using std::ostream;

#include "VariantUnion.h"
#include "RefCountedPointer.h"

//=============================================================================
class Variant : public RefCountedPointer
{
public:
  // return a new object of the same type
  virtual Variant *NewInstance()=0;
  //virtual Variant *Duplicate()=0;

  // // reference counting machinery, derived classes should
  // // implement New, and define constructors protected.
  // virtual void Delete()=0;
  // virtual void Register()=0;

  virtual ~Variant()=0;

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

  // Logical (return the result in a new Variant)
  virtual Variant *Equal(Variant *rhs)=0;
  virtual Variant *NotEqual(Variant *rhs)=0;
  virtual Variant *Less(Variant *rhs)=0;
  virtual Variant *LessEqual(Variant *rhs)=0;
  virtual Variant *Greater(Variant *rhs)=0;
  virtual Variant *GreaterEqual(Variant *rhs)=0;
  virtual Variant *And(Variant *rhs)=0;
  virtual Variant *Or(Variant *rhs)=0;
  virtual Variant *Not()=0;

  // Arithmetic (return the result in a new Variant)
  virtual Variant *Add(Variant *rhs)=0;
  virtual Variant *Subtract(Variant *rhs)=0;
  virtual Variant *Multiply(Variant *rhs)=0;
  virtual Variant *Divide(Variant *rhs)=0;
  virtual Variant *Sqrt()=0;
  virtual Variant *Negate()=0;
  // In-place arithmetic (return pointer to this)
  virtual Variant *AddAssign(Variant *rhs)=0;
  virtual Variant *SubtractAssign(Variant *rhs)=0;
  virtual Variant *MultiplyAssign(Variant *rhs)=0;
  virtual Variant *DivideAssign(Variant *rhs)=0;
  virtual Variant *Assign(Variant *rhs)=0;

  // Vector arirthmetic (return result in a new Variant)
  virtual Variant *Component(int i)=0;
  virtual Variant *L2Norm()=0;

  // Type information
  virtual int GetTypeId()=0;
  virtual const char *GetTypeString()=0;

  // Print the value and type.
  virtual void Print(ostream &os)=0;

  // The value.
  VariantUnion Data;
};

Variant::~Variant(){}

//-----------------------------------------------------------------------------
ostream &operator<<(ostream &os,Variant &v)
{
  v.Print(os);
  return os;
}

#endif
