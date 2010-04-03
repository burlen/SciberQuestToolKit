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
class VariantStack;
class TokenParser;

//=============================================================================
class Token : public RefCountedPointer
{
public:
  virtual ~Token();

  // Decsription:
  // Token meta-data.
  virtual int GetTypeId()=0;
  virtual const char *GetTypeString()=0;

  // Description:
  // Recursive parser stubs, nud is called for prefix operators
  // and operands. led is called for infix and postfix operators.
  virtual void Nud();
  virtual void Led();

  // Description:
  // Binding priorities, determinie operator precedence. See 
  // TokenBindingPower enum below for the specifics.
  virtual int Nbp(){ return 0; }
  virtual int Lbp(){ return 0; }

  // Description:
  // Tokens representing operators pop their arguments operate 
  // and push the reult back onto the stack.
  virtual void Operate(VariantStack *Stack);

  // Description
  // Set/Get interanl data associated with the token. Mostly of
  // concern for literals.
  SetRefCountedPointer(Value,Variant);
  virtual Variant *GetValue(){ return this->Value; }

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
TokenBindingPower;

#endif
