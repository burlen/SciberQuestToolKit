/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/

#include "TokenList.h"

#include "Token.h"

//-----------------------------------------------------------------------------
void TokenList::Append(Token *t)
{
  t->Register();
  this->Tokens.push_back(t);
}

//-----------------------------------------------------------------------------
void TokenList::Clear()
{
  size_t n=this->Tokens.size();
  for (size_t i=0; i<n; ++i)
    {
    this->Tokens[i]->Delete();
    }
  this->Tokens.clear();
}

//-----------------------------------------------------------------------------
void TokenList::Print(ostream &os)
{
  os << "TokenList : ";
  size_t n=this->Tokens.size();
  for (size_t i=0; i<n; ++i)
    {
    this->Tokens[i]->Print(os);
    }
}

//*****************************************************************************
ostream &operator<<(ostream &os, TokenList &tl)
{
  tl.Print(os);
  return os;
}
