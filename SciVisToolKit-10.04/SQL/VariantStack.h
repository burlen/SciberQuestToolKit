/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef VariantStack_h
#define VariantStack_h

#include <ostream>
using std::ostream;
#include <deque>
using std::deque;

#include "RefCountedPointer.h"

class Variant;
//=============================================================================
class VariantStack : public RefCountedPointer
{
public:
  static VariantStack *New(){ return new VariantStack; }

  // Description:
  // Append tokens to the list to be parsed.
  void Push(Variant *t);

  // Description:
  // Append tokens to the list to be parsed, its reference count is not
  // incremented, but will be later decremented druing Clear. Use this
  // like AppendNewVariant(VariantX::New());
  void PushNew(Variant *t){ this->Stack.push_back(t); }

  // Description:
  // remove the element at the top of the stack. The object will be marked 
  // for Delete'tion in the destructor.
  Variant *Pop()
    {
    Variant *v=this->Stack.back();
    this->Stack.pop_back();
    this->Garbage.push_back(v);
    return v;
    }

  // Description:
  // Access the element at the top of the stack.
  Variant *Top(){ this->Stack.back(); }

  // Description:
  // Clear the list and delete it's contents.
  void Clear();

  // Descrition:
  // Clear any poped elements. As elements are poped their ref count
  // isn't decremented, instead they are gathered for later decrement.
  void ClearGarbage();

  // Description:
  // Print the contents of the list.
  virtual void Print(ostream &os);

protected:
  VariantStack(){}
  ~VariantStack(){ this->Clear(); }

private:
  deque<Variant*> Stack;
  deque<Variant*> Garbage;
};

//*****************************************************************************
ostream &operator<<(ostream &os, VariantStack &tl);

#endif
