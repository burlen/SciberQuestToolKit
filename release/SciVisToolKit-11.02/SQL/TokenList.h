/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef TokenList_h
#define TokenList_h

#include <ostream>
using std::ostream;
#include <deque>
using std::deque;

#include "RefCountedPointer.h"

class Token;
//=============================================================================
class TokenList : public RefCountedPointer
{
public:
  static TokenList *New(){ return new TokenList; }

  // Description:
  // Append tokens to the list to be parsed.
  void Append(Token *t);

  // Description:
  // Append tokens to the list to be parsed, its reference count is not
  // incremented, but will be later decremented druing Clear. Use this
  // like AppendNewToken(TokenX::New());
  void AppendNew(Token *t){ this->Tokens.push_back(t); }

  // Description:
  // Clear the list and delete it's contents.
  void Clear();

  // Description:
  // Token iterator
  void IteratorBegin(){ this->At=0; }
  void IteratorIncrement(){ ++this->At; }
  bool IteratorOk(){ return this->At<this->Tokens.size(); }
  Token *GetCurrent(){ return this->Tokens[this->At]; }

  // Description:
  // Print the contents of the list.
  virtual void Print(ostream &os);

protected:
  TokenList()
     :
    At(0)
    { }

  ~TokenList(){ this->Clear(); }

private:
  TokenList(const TokenList &); // not implemented
  TokenList &operator=(const TokenList &);

private:
  deque<Token*> Tokens;
  int At;
};

//*****************************************************************************
ostream &operator<<(ostream &os, TokenList &tl);

#endif
