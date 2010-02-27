/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef RefCountedPointer_h
#define RefCountedPointer_h

#include <iostream>
using std::ostream;
using std::cerr;
using std::endl;

class RefCountedPointer
{
public:
  RefCountedPointer() : N(1) {}
  virtual void Delete(){ if ((--this->N)==0){ delete this;} }
  virtual void Register(){ ++this->N; }
  virtual int RefCount(){ return this->N; }
  virtual ~RefCountedPointer(){}
  virtual void Print(ostream &os){ /*os << "N=" << this->N;*/ }
private:
  int N;
};

#define SetRefCountedPointer(NAME,TYPE)\
  virtual void Set##NAME(TYPE *v)\
    {\
    if (v==this->NAME)return;\
    if (this->NAME) this->NAME->Delete();\
    this->NAME=v;\
    if (this->NAME) this->NAME->Register();\
    }

#endif
