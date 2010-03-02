/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef Tokens_h
#define Tokens_h

#include "Token.h"
#include "TokenList.h"
#include "TokenParser.h"
#include "Variants.h"
#include "VariantStack.h"

#define TerminationTokenImpl(NAME)\
/*=============================================================================*/\
class NAME##Terminator : public Token\
{\
public:\
  static NAME##Terminator *New(TokenParser *p){ return new NAME##Terminator(p); }\
\
  virtual const char *GetTypeString(){ return #NAME "Terminator"; }\
  virtual int GetTypeId(){ return TERMINATOR_TYPE; }\
\
  virtual int Nbp(){ return TERMINATOR_BP; }\
  virtual int Lbp(){ return TERMINATOR_BP; }\
\
protected:\
  NAME##Terminator(TokenParser *p)\
      :\
    Token(0,p)\
      {}\
private:\
  NAME##Terminator(const NAME##Terminator &);\
  NAME##Terminator &operator=(const NAME##Terminator &);\
};

TerminationTokenImpl(Program) // EOF
TerminationTokenImpl(Bracket) // ]
TerminationTokenImpl(Paren)   // )


//=============================================================================
class Operand : public Token
{
public:
  static Operand *New(double d, TokenParser *p);
  static Operand *New(Variant *v, TokenParser *p);

  virtual const char *GetTypeString(){ return "Operand"; }
  virtual int GetTypeId(){ return OPERAND_TYPE; }

  virtual int Lbp(){ return OPERAND_BP; }

  virtual void Nud()
    {
    this->Parser->GetByteCode()->Append(this);

    /// Variant *v=this->GetValue();
    /// v->Register();
    /// cerr << " " << *v;
    /// return v; 
    }

  virtual void Operate(VariantStack *Stack);

protected:
  Operand(Variant *v, TokenParser *p)
      :
    Token(v,p)
      {}
private:
  Operand();
  Operand(const Operand &);
  Operand &operator=(const Operand &);
};

//-----------------------------------------------------------------------------
Operand *Operand::New(double d, TokenParser *p)
{
  Variant *v=DoubleVariant::New(d);
  Operand *o=new Operand(v,p);
  v->Delete();
  return o;
}

//-----------------------------------------------------------------------------
void Operand::Operate(VariantStack *Stack)
{
  Variant *v=this->GetValue();
  Stack->Push(v);
  cerr << "(" << *v << ")" << endl;
}

//-----------------------------------------------------------------------------
Operand *Operand::New(Variant *v, TokenParser *p)
{
  return new Operand(v,p);
}


#define InfixOperatorImpl(OPERATOR_NAME,OPERATOR_BP,BP_REDUCTION)\
/*=============================================================================*/\
class OPERATOR_NAME : public Token\
{\
public:\
  static OPERATOR_NAME *New(TokenParser *p){ return new OPERATOR_NAME(p); }\
\
  virtual const char *GetTypeString(){ return #OPERATOR_NAME; }\
  virtual int GetTypeId(){ return INFIX_OPERATOR_TYPE; }\
\
  virtual int Lbp(){ return OPERATOR_BP; }\
\
  virtual void Led();\
\
  virtual void Operate(VariantStack *Stack);\
\
protected:\
  OPERATOR_NAME(TokenParser *p)\
      :\
    Token(0,p)\
      {}\
private:\
  OPERATOR_NAME();\
  OPERATOR_NAME(const OPERATOR_NAME &);\
  OPERATOR_NAME &operator=(const OPERATOR_NAME &);\
};\
\
/*-----------------------------------------------------------------------------*/\
void OPERATOR_NAME::Led()\
{\
  this->GetTokenParser()->Parse(this->Lbp() BP_REDUCTION);\
  this->Parser->GetByteCode()->Append(this);\
  /**Variant *right=this->GetTokenParser()->Parse(this->Lbp() BP_REDUCTION);*/\
  /**cerr << " " #OPERATOR_NAME;*/\
  /**return left->OPERATOR_NAME(right);*/\
}\
\
/*-----------------------------------------------------------------------------*/\
void OPERATOR_NAME::Operate(VariantStack *Stack)\
{\
  Variant *right=Stack->Pop();\
  Variant *left=Stack->Pop();\
  Stack->PushNew(left->OPERATOR_NAME(right));\
  cerr << "(" << *left << " " << *right << " " #OPERATOR_NAME << ")" << endl;\
}

InfixOperatorImpl(Add,ADDITION_BP,)
InfixOperatorImpl(Subtract,ADDITION_BP,)
InfixOperatorImpl(Multiply,MULTIPLICATION_BP,)
InfixOperatorImpl(Divide,MULTIPLICATION_BP,)

InfixOperatorImpl(Equal,EQUIVALENCE_BP,)
InfixOperatorImpl(NotEqual,EQUIVALENCE_BP,)
InfixOperatorImpl(Less,EQUIVALENCE_BP,)
InfixOperatorImpl(LessEqual,EQUIVALENCE_BP,)
InfixOperatorImpl(Greater,EQUIVALENCE_BP,)
InfixOperatorImpl(GreaterEqual,EQUIVALENCE_BP,)

InfixOperatorImpl(And,LOGICAL_BP,-1)
InfixOperatorImpl(Or,LOGICAL_BP,-1)

//=============================================================================
class Bracket : public Token
{
public:
  static Bracket *New(TokenParser *p){ return new Bracket(p); }

  virtual const char *GetTypeString(){ return "Bracket"; }
  virtual int GetTypeId(){ return INFIX_OPERATOR_TYPE; }

  virtual int Lbp(){ return GROUPING_BP; }

  virtual void Led();

protected:
  Bracket(TokenParser *p)
      :
    Token(0,p)
      {}
private:
  Bracket();
  Bracket(const Bracket &);
  Bracket &operator=(const Bracket &);
};

//-----------------------------------------------------------------------------
void Bracket::Led()
{
  ///Variant *right=this->GetTokenParser()->Parse(OPERAND_BP);
  /// for now assume constant only. if this is the result of a
  /// complex expression then we do not want to increment the
  /// ref count.
  ///right->Register();

  this->GetTokenParser()->Parse(OPERAND_BP);

  // Validate that there is a ] token next in line and move
  // past it.
  Token *t=this->Parser->GetProgram()->GetCurrent();
  if (!dynamic_cast<BracketTerminator*>(t))
    {
    SyntaxError e;
    throw e;
    }
  this->Parser->GetProgram()->IteratorIncrement();

  // TODO Push Component operator on tho the bytecode. I think the index is pushed...

  ///return left->Component(right);
  ///return 0;
}

#define PrefixOperatorImpl(OPERATOR_NAME,OPERATOR_BP)\
/*=============================================================================*/\
class OPERATOR_NAME : public Token\
{\
public:\
  static OPERATOR_NAME *New(TokenParser *p){ return new OPERATOR_NAME(p); }\
\
  virtual const char *GetTypeString(){ return #OPERATOR_NAME; }\
  virtual int GetTypeId(){ return PREFIX_OPERATOR_TYPE; }\
\
  virtual int Nbp(){ return OPERATOR_BP; }\
\
  virtual void Nud();\
\
  virtual void Operate(VariantStack *Stack);\
\
protected:\
  OPERATOR_NAME(TokenParser *p)\
      :\
    Token(0,p)\
      {}\
private:\
  OPERATOR_NAME();\
  OPERATOR_NAME(const OPERATOR_NAME &);\
  OPERATOR_NAME &operator=(const OPERATOR_NAME &);\
};\
\
/*-----------------------------------------------------------------------------*/\
void OPERATOR_NAME::Nud()\
{\
  this->GetTokenParser()->Parse(this->Nbp());\
  this->Parser->GetByteCode()->Append(this);\
  /**Variant *right=this->GetTokenParser()->Parse(this->Nbp());*/\
  /**cerr << " " #OPERATOR_NAME ;*/\
  /**return right->OPERATOR_NAME();*/\
}\
\
/*-----------------------------------------------------------------------------*/\
void OPERATOR_NAME::Operate(VariantStack *Stack)\
{\
  Variant *right=Stack->Pop();\
  Stack->PushNew(right->OPERATOR_NAME());\
  cerr << "(" << *right <<  " " #OPERATOR_NAME << ")" << endl;\
}

PrefixOperatorImpl(Negate,UNARY_BP)
PrefixOperatorImpl(Not,UNARY_BP)


// Minus is an infix token that ecompasses both subtract and negate.
//=============================================================================
class Minus : public Token
{
public:
  static Minus *New(TokenParser *p){ return new Minus(p); }

  virtual const char *GetTypeString(){ return "Minus"; }
  virtual int GetTypeId(){ return INFIX_OPERATOR_TYPE; }

  virtual int Nbp(){ return UNARY_BP; }
  virtual int Lbp(){ return ADDITION_BP; }

  virtual void Nud();
  virtual void Led();

protected:
  Minus(TokenParser *p)
      :
    Token(0,p)
      {}
private:
  Minus();
  Minus(const Minus &);
  Minus &operator=(const Minus &);
};

//-----------------------------------------------------------------------------
void Minus::Nud()
{
  this->GetTokenParser()->Parse(this->Nbp());

  Negate *neg=Negate::New(this->Parser);
  this->Parser->GetByteCode()->Append(neg);
  neg->Delete();

  /// Variant *right=this->GetTokenParser()->Parse(this->Nbp());
  /// cerr << " Negate";
  /// return right->Negate();
}

//-----------------------------------------------------------------------------
void Minus::Led()
{
  this->GetTokenParser()->Parse(this->Lbp());

  Subtract *sub=Subtract::New(this->Parser);
  this->Parser->GetByteCode()->Append(sub);
  sub->Delete();

  /// Variant *right=this->GetTokenParser()->Parse(this->Lbp());
  /// cerr << " Subtract";
  /// return left->Subtract(right);
}




//=============================================================================
class Paren : public Token
{
public:
  static Paren *New(TokenParser *p){ return new Paren(p); }

  virtual const char *GetTypeString(){ return "Paren"; }
  virtual int GetTypeId(){ return INFIX_OPERATOR_TYPE; }

  virtual int Nbp(){ return GROUPING_BP; }

  virtual void Nud();

protected:
  Paren(TokenParser *p)
      :
    Token(0,p)
      {}
private:
  Paren();
  Paren(const Paren &);
  Paren &operator=(const Paren &);
};

//-----------------------------------------------------------------------------
void Paren::Nud()
{
  this->GetTokenParser()->Parse(OPERAND_BP);
  // Validate that there is a ) token next in line and move
  // past it.
  Token *t=this->Parser->GetProgram()->GetCurrent();
  if (!dynamic_cast<ParenTerminator*>(t))
    {
    SyntaxError e;
    throw e;
    }
  this->Parser->GetProgram()->IteratorIncrement();

  /// Variant *right=this->GetTokenParser()->Parse(OPERAND_BP);
  /// for now assume the result is the result of a complex expression
  /// if it were constant then we'd have to increment its refcount to
  /// avoid double free.
  /// right->Register();

  /// Validate that there is a ) token next in line and move
  /// past it.
  /// Token *t=this->Parser->GetProgram()->GetCurrent();
  /// if (!dynamic_cast<ParenTerminator*>(t))
  ///   {
  ///   SyntaxError e;
  ///   throw e;
  ///   }
  /// this->Parser->GetProgram()->IteratorIncrement();

  /// return right;
}

#endif
