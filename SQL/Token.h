/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef Token_h
#define Token_h

#include <iostream>
using std::ostream;

#include "RefCountedPointer.h"
class Variant;
class TokenParser;

//=============================================================================
class Token : public RefCountedPointer
{
public:
  virtual ~Token();

  virtual int GetTypeId()=0;
  virtual const char *GetTypeString()=0;

  // NOTE if a new value is not returned then
  // its reference count must be incremented.
  virtual Variant *Nud();
  virtual Variant *Led(Variant *left);

  virtual int Nbp(){ return 0; }
  virtual int Lbp(){ return 0; }

  // Description
  // Set/Get interanl data associated with the token. Mostly of
  // concern for literals.
  SetRefCountedPointer(Value,Variant);
  virtual Variant *GetVariant(){ return this->Value; }

  // Set/Get the parser that operates on the tokens, tokens
  // representing operators modify the parse to get their 
  // operands etc etc.
  SetRefCountedPointer(Parser,TokenParser);
  virtual TokenParser *GetTokenParser(){ return this->Parser; }

  virtual void Print(ostream &os);

protected:
  Token();
  Token(Variant *v, TokenParser *p);

  Variant *Value;
  TokenParser *Parser;
};

//*****************************************************************************
ostream &operator<<(ostream &os, Token &t);

/// Token constants
typedef enum 
  {
  TERMINATOR_TYPE,
  OPERAND_TYPE,
  INFIX_OPERATOR_TYPE,
  PREFIX_OPERATOR_TYPE
  }
TokenTypeId;

typedef enum
  {
  TERMINATOR_BP=0,
  OPERAND_BP=0,
  LOGICAL_BP=30,
  EQUIVALENCE_BP=40,
  ADDITION_BP=50,
  MULTIPLICATION_BP=60,
  UNARY_BP=70,
  GROUPING_BP=80
  }
TokenLeftBindingPower;

#endif
