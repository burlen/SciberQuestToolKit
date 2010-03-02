/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#include "Token.h"
#include "Variant.h"
#include "TokenParser.h"

using std::cerr;
using std::endl;

//*****************************************************************************
ostream &operator<<(ostream &os, Token &t)
{
  t.Print(os);
  return os;
}

//-----------------------------------------------------------------------------
SetRefCountedPointerImpl(Token,Value,Variant);

//-----------------------------------------------------------------------------
SetRefCountedPointerImpl(Token,Parser,TokenParser);

//-----------------------------------------------------------------------------
Token::Token()
  :
  Value(0),
  Parser(0)
{}

//-----------------------------------------------------------------------------
Token::Token(Variant *v, TokenParser *p)
    :
  Value(0),
  Parser(0)
{
  this->SetValue(v);
  this->SetParser(p);
}

//-----------------------------------------------------------------------------
Token::~Token()
{
  this->SetValue(0);
  this->SetParser(0);
}

//-----------------------------------------------------------------------------
Variant *Token::Nud()
{
  cerr << "Warning: " << GetTypeString() << "::Nud was not implemented." << endl;
  return 0;
}

//-----------------------------------------------------------------------------
Variant *Token::Led(Variant *left)
{
  cerr << "Warning: " << GetTypeString() << "::Led was not implemented." << endl;
  return 0;
}

//-----------------------------------------------------------------------------
void Token::Print(ostream &os)
{
  this->RefCountedPointer::Print(os); cerr << " ";

  os << this->GetTypeString();
  Variant *v=this->Value;
  if (v)
    {
    os << " " << *v;
    }
}
