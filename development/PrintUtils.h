/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef PrintUtils_h
#define PrintUtils_h
//BTX
#include<iostream>
#include<map>
#include<vector>

#if defined PV_3_4_BUILD
  #include "vtkAMRBox_3.7.h"
#else
  #include "vtkAMRBox.h"
#endif

using namespace std;
ostream &operator<<(ostream &os, const map<string,int> &m);
ostream &operator<<(ostream &os, const vector<vtkAMRBox> &v);
ostream &operator<<(ostream &os, const vector<string> &v);
ostream &operator<<(ostream &os, const vector<double> &v);
ostream &operator<<(ostream &os, const vector<int> &v);
ostream &PrintD3(ostream &os, const double *v3);
ostream &PrintD6(ostream &os, const double *v6);
ostream &PrintI3(ostream &os, const int *v3);
ostream &PrintI6(ostream &os, const int *v6);
inline
const char *safeio(const char *s){ return (s?s:"NULL"); }
//ETX
#endif
