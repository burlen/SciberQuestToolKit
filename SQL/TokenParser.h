/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef TokenParser_h
#define TokenParser_h

#include <iostream>
using std::ostream;
#include <vector>
using std::vector;
#include <exception>
using std::exception;

#include "RefCountedPointer.h"

//=============================================================================
class SyntaxError : public exception
{
  virtual const char* what() const throw()
    {
    return "Syntax Error";
    }
};

class Token;
class Variant;
//=============================================================================
class TokenParser : public RefCountedPointer
{
public:
  static TokenParser *New(){ return new TokenParser; }

  // Description:
  // Append tokens to the list to be parsed.
  void AppendToken(Token *t){ this->Tokens.push_back(t); }

  // Description:
  // Convinience method that calls Delete on all of the tokens,
  // By default their ref-count is incremented in the append call
  // so use this only if needed.
  void DeleteTokens();

  // Description:
  // Token iterator
  void IteratorBegin(){ this->At=0; }
  void IteratorIncrement(){ ++this->At; }
  bool IteratorOk(){ this->At<this->Tokens.size(); }
  Token *GetCurrentToken(){ return this->Tokens[this->At]; }

  Variant *Parse(int rbp);

  void CollectGarbage();

protected:
  TokenParser()
     :
    At(0)
    {}
  ~TokenParser();

private:
  TokenParser(const TokenParser &); // not implemented
  TokenParser &operator=(const TokenParser &);

private:
  vector<Token*> Tokens;
  vector<Variant*> Garbage;
  int At;
};

#endif
