/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef Values_h
#define Values_h

#include "ValueTemplate.h"
#include "ManagedPointer.h"

typedef ValueTemplate<char>       CharValue;
typedef ValueTemplate<bool>       BoolValue;
typedef ValueTemplate<int>        IntValue;
typedef ValueTemplate<float>      FloatValue;
typedef ValueTemplate<double>     DoubleValue;
typedef ValueTemplate<long long>  LongLongValue;

typedef ValueTemplate<char*>      CharValuePointer;
typedef ValueTemplate<bool*>      BoolValuePointer;
typedef ValueTemplate<int*>       IntValuePointer;
typedef ValueTemplate<float*>     FloatValuePointer;
typedef ValueTemplate<double*>    DoubleValuePointer;
typedef ValueTemplate<long long*> LongLongValuePointer;

typedef ManagedPointer<Value>     PValue;

#endif
