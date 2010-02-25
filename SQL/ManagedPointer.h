/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef ManagedPointer_h
#define ManagedPointer_h


template<typename T>
class ManagedPointer
{
public:
  ManagedPointer()
     :
    P(0)
    {}
  ManagedPointer(T *p)
     :
    P(p)
    {}
  ~ManagedPointer()
    {
    if (P) P->Delete();
    }
  // cast to the pointee
  operator T*()
    {
    return this->P;
    }

  // Copy a reference to. Increments ref count.
  T *operator=(ManagedPointer<T> &other)
    {
    if (this==&other) return this->P;
    if (other.P==this->P) return this->P;
    if (this->P) P->Delete();
    this->P=other.P;
    if (this->P) this->P->Register();
    }

  // Description:
  // Assign ownership, ref count is not incremented.
  T *operator=(T *p)
    {
    if (p==this->P) return this->P;
    if (this->P) P->Delete();
    this->P=p;
    //this->P->Register();
    }

  // Member operator
  T *operator->()
    {
    return this->P;
    }
  T &operator*()
    {
    return *this->P;
    }

private:
  T *P;
};

#endif
