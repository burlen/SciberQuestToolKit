/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef Variants_h
#define Variants_h

#include "VariantTemplate.h"
#include "ManagedPointer.h"

typedef VariantTemplate<char>       CharVariant;
typedef VariantTemplate<bool>       BoolVariant;
typedef VariantTemplate<int>        IntVariant;
typedef VariantTemplate<float>      FloatVariant;
typedef VariantTemplate<double>     DoubleVariant;
typedef VariantTemplate<long long>  LongLongVariant;

typedef VariantTemplate<char*>      CharVariantPointer;
typedef VariantTemplate<bool*>      BoolVariantPointer;
typedef VariantTemplate<int*>       IntVariantPointer;
typedef VariantTemplate<float*>     FloatVariantPointer;
typedef VariantTemplate<double*>    DoubleVariantPointer;
typedef VariantTemplate<long long*> LongLongVariantPointer;

typedef ManagedPointer<Variant>     MP_Variant;

#endif
