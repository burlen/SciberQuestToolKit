/*   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef fsutil_h
#define fsutil_h

#include<vector>
#include<string>
using std::string;
using std::vector;

int Represented(const char *path, const char *prefix);
int GetSeriesIds(const char *path, const char *prefix, vector<int> &ids);
string StripFileNameFromPath(const string fileName);
int LoadLines(const char *fileName, vector<string> &lines);
int WriteText(string &fileName, string &text);
int SearchAndReplace(const string &searchFor,const string &replaceWith,string &inText);
ostream &operator<<(ostream &os, vector<string> v);
bool operator&(vector<string> &v, const string &s);

#endif
