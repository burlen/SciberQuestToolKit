/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __CellIdBlock_h
#define __CellIdBlock_h

#include <iostream>
using std::ostream;

/// A block of adjecent indexes into a collection of cells.
class CellIdBlock
{
public:
  CellIdBlock()
    {
    this->clear();
    }
  void clear(){ m_data[0]=m_data[1]=0; }
  int &first(){ return m_data[0]; }
  int &size(){ return m_data[1]; }
  int last(){ return m_data[0]+m_data[1]; }
  int *data(){ return m_data; }
  int dataSize(){ return 2; }
  bool contains(int id){ return ((id>=first()) && (id<last())); }
private:
  int m_data[2];
private:
  friend ostream &operator<<(ostream &os, const CellIdBlock &b);
};

#endif
