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
    {
    if (this->P){ this->P->Register();  cerr << " REGISTER " << *this->P;}
    }
  ManagedPointer(T *p, int addRef)
     :
    P(p)
    {
    if (addRef && this->P){ this->P->Register();  cerr << " REGISTER " << *this->P; }
    }
  ManagedPointer(ManagedPointer &other)
     :
    P(0)
    {
    if (this==&other) return;
    this->operator=(other);
    }
  ~ManagedPointer()
    {
    if (P) { cerr << " DELETE " << *this->P; this->P->Delete();}
    }

  // return a smart pointer that owns a new object T
  static ManagedPointer<T> New()
    {
    return ManagedPointer<T>(T::New(),0); 
    }

  // cast to the pointee
  operator T*()
    {
    return this->P;
    }

  // Copy a reference to
  T *operator=(ManagedPointer<T> &other)
    {
    if (this==&other) return this->P;
    return this->operator=(other.P);
    }

  // Description:
  // Assign ownership
  T *operator=(T *p)
    {
    if (p==this->P)
      {
      this->P->Register();
      cerr << " REGISTER " << *this->P;
      return this->P;
      }
    if (this->P) { cerr << " DELETE " << *this->P; this->P->Delete();}
    this->P=p;
    if (this->P) { this->P->Register();  cerr << " REGISTER " << *this->P;}
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
