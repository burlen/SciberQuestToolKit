/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/

#include "TokenParser.h"
#include "Token.h"
#include "Variant.h"


//-----------------------------------------------------------------------------
void TokenParser::DeleteTokens()
{
  size_t n=this->Tokens.size();
  for (size_t i=0; i<n; ++i)
    {
    this->Tokens[i]->Delete();
    }
  this->Tokens.clear();
}

//-----------------------------------------------------------------------------
void TokenParser::CollectGarbage()
{
  size_t n=this->Garbage.size();
  for (size_t i=0; i<n; ++i)
    {
    this->Garbage[i]->Delete();
    }
  this->Garbage.clear();
}

//-----------------------------------------------------------------------------
TokenParser::~TokenParser()
{
  cerr << " ~TokenParser" << endl;
  this->CollectGarbage();
}

//-----------------------------------------------------------------------------
Variant *TokenParser::Parse(int rbp)
{
  // cerr << " Parse(rbp=" << rbp << ") : ";
  Variant *left;
  Token *current=this->GetCurrentToken();
  // cerr << *current;
  this->IteratorIncrement();
  left=current->Nud();
  this->Garbage.push_back(left);
  // if (left) cerr << " " << *left;

  while (rbp<this->GetCurrentToken()->Lbp())
    {
    current=this->GetCurrentToken();
    // cerr << " : " << *current << endl;
    this->IteratorIncrement();
    left=current->Led(left);
    this->Garbage.push_back(left);
    }
  // cerr << " " << *current;
  // if (left) cerr << " " << *left;
  return left;
}

//     def expression(rbp=0):
//         global token
//         t = token
//         token = next()
//         left = t.nud()
//         while rbp < token.lbp:
//             t = token
//             token = next()
//             left = t.led(left)
//         return left
