/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef VariantUnion_h
#define VariantUnion_h

typedef union
    {
    char Char;
    char *CharPtr;
    bool Bool;
    bool *BoolPtr;
    int Int;
    int *IntPtr;
    float Float;
    float *FloatPtr;
    double Double;
    double *DoublePtr;
    long long LongLong;
    long long *LongLongPtr;
    }
VariantUnion;

#endif
