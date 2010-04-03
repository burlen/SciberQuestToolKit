/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/

#include "VariantStack.h"

#include "Variant.h"

//-----------------------------------------------------------------------------
void VariantStack::Push(Variant *t)
{
  t->Register();
  this->Stack.push_back(t);
}

//-----------------------------------------------------------------------------
void VariantStack::ClearGarbage()
{
  size_t n=this->Garbage.size();
  for (size_t i=0; i<n; ++i)
    {
    this->Garbage[i]->Delete();
    }
  this->Garbage.clear();
}

//-----------------------------------------------------------------------------
void VariantStack::Clear()
{
  this->ClearGarbage();

  size_t n=this->Stack.size();
  for (size_t i=0; i<n; ++i)
    {
    this->Stack[i]->Delete();
    }
  this->Stack.clear();
}

//-----------------------------------------------------------------------------
void VariantStack::Print(ostream &os)
{
  os << "VariantStack : ";
  size_t n=this->Stack.size();
  for (size_t i=0; i<n; ++i)
    {
    this->Stack[i]->Print(os);
    }
}

//*****************************************************************************
ostream &operator<<(ostream &os, VariantStack &tl)
{
  tl.Print(os);
  return os;
}
