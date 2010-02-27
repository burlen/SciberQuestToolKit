#include <iostream>
using std::cerr;
using std::endl;
#include <vector>
using std::vector;

#include <exception>
using std::exception;

#include "Values.h"

#include "RefCountedPointer.h"

/// Token traits
typedef enum 
  {
  TERMINATOR_TYPE,
  OPERAND_TYPE,
  OR_TYPE,
  AND_TYPE,
  EQUAL_TYPE,
  ADD_TYPE,
  SUBTRACT_TYPE,
  MULTIPLY_TYPE,
  DIVIDE_TYPE,
  NEGATE_TYPE,
  NOT_TYPE
  }
TokenTypeId;

typedef enum
  {
  TERMINATOR_LBP=0,
  OPERAND_LBP=0,
  OR_LBP=30,
  AND_LBP=30,
  EQUAL_LBP=40,
  NOT_EQUAL_LBP=40,
  ADD_LBP=50,
  MULTIPLY_LBP=60,
  NEGATE_LBP=70,
  GROUP_LBP=80
  }
TokenLeftBindingPower;




/// Expression

//=============================================================================
class SyntaxError : public exception
{
  virtual const char* what() const throw()
    {
    return "Syntax Error";
    }
};

//   try
//   {
//     throw myex;
//   }
//   catch (exception& e)
//   {
//     cout << e.what() << endl;
//   }

class Token;
//=============================================================================
class TokenTokenParser : public RefCountedPointer
{
public:
  static TokenParser *New(){ return new TokenParser; }

  // Description:
  // Append tokens to the list to be parsed.
  void Append(Token *t){ this->Tokens.push_back(t); }

  // Description:
  // Token iterator
  void IteratorBegin(){ this->At=0; }
  void IteratorIncrement(){ ++this->At; }
  bool IteratorOk(){ this->At<this->Tokens.size(); }

  Value *Parse(int rbp);

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
  vector<Value *> Garbage;
  int At;
};



/// Token interface
//=============================================================================
class Token : public RefCountedPointer
{
public:
  virtual int GetTypeId()=0;
  virtual const char *GetTypeString()=0;

  // NOTE if a new value is not returned then
  // its reference count must be incremented.
  virtual Value *Nud();
  virtual Value *Led(Value *left);

  virtual int Lbp(){ return 0; }

  SetRefCountedPointer(Value,Value);
  virtual Value *GetValue(){ return this->V; }

  virtual void SetTokenParser(TokenParser *p){ this->P=p; }
  virtual TokenParser *GetTokenParser(){ return this->P; }

  virtual void Print(ostream &os);

  virtual ~Token();

protected:
  Token();
  Token(Value *v, TokenParser *p);

  Value *Value;
  TokenParser *Parser;
};

//-----------------------------------------------------------------------------
ostream &operator<<(ostream &os, Token &t)
{
  t.Print(os);
  return os;
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

  size_t n=this->Tokens.size();
  for (size_t i=0; i<n; ++i)
    {
    this->Tokens[i]->Delete();
    }
}

//-----------------------------------------------------------------------------
Value *TokenParser::Parse(int rbp)
{
  Value *left;
  Token *current=this->Tokens[this->At];
  cerr << "Parse : " << *current;
  ++this->At;
  left=current->Nud();
  this->Garbage.push_back(left);

  while (rbp<this->Tokens[this->At]->Lbp())
    {
    current=this->Tokens[this->At];
    cerr << " : " << *current << endl;
    ++this->At;
    left=current->Led(left);
    this->Garbage.push_back(left);
    }
  cerr << " : Ret";
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


//-----------------------------------------------------------------------------
Token::Token()
  :
  V(0),
  P(0)
{}

//-----------------------------------------------------------------------------
Token::Token(Value *v, TokenParser *p)
    :
  V(0),
  P(0)
{
  this->SetValue(v);
  this->SetTokenParser(p);
}

//-----------------------------------------------------------------------------
Token::~Token()
{
  this->SetValue(0);
  this->SetTokenParser(0);
}

//-----------------------------------------------------------------------------
Value *Token::Nud()
{
  cerr << "Warning: " << GetTypeString() << "::Nud was not implemented." << endl;
  return 0;
}

//-----------------------------------------------------------------------------
Value *Token::Led(Value *left)
{
  cerr << "Warning: " << GetTypeString() << "::Led was not implemented." << endl;
  return 0;
}

//-----------------------------------------------------------------------------
void Token::Print(ostream &os)
{
  this->RefCountedPointer::Print(os); cerr << " ";

  os << this->GetTypeString();
  Value *v=this->V;
  if (v)
    {
    os << " " << *v;
    }
}


/// Token specializations
//=============================================================================
class Terminator : public Token
{
public:
  static Terminator *New(){ return new Terminator; }

  virtual const char *GetTypeString(){ return "Terminator"; }
  virtual int GetTypeId(){ return TERMINATOR_TYPE; }
  virtual int Lbp(){ return TERMINATOR_LBP; }

protected:
  Terminator()
      :
    Token(0,0)
      {}
private:
  Terminator(const Terminator &);
  Terminator &operator=(const Terminator &);
};

//=============================================================================
class Operand : public Token
{
public:
  static Operand *New(double d, TokenParser *p);
  static Operand *New(Value *v, TokenParser *p);

  virtual const char *GetTypeString(){ return "Operand"; }
  virtual int GetTypeId(){ return OPERAND_TYPE; }

  virtual int Lbp(){ return OPERAND_LBP; }

  virtual Value *Nud()
    {
    Value *v=this->GetValue();
    v->Register();
    return v; 
    }

protected:
  Operand(Value *v, TokenParser *p)
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
  Value *v=DoubleValue::New(d);
  Operand *o=new Operand(v,p);
  v->Delete();
  return o;
}

//-----------------------------------------------------------------------------
Operand *Operand::New(Value *v, TokenParser *p)
{
  return new Operand(v,p);
}


#define InfixOperator(OPERATOR_NAME,OPERATOR_TYPE_ID,OPERATOR_LBP)\
/*=============================================================================*/\
class Operator##OPERATOR_NAME : public Token\
{\
public:\
  static Operator##OPERATOR_NAME *New(TokenParser *p){ return new Operator##OPERATOR_NAME(p); }\
\
  virtual const char *GetTypeString(){ return "Operator" #OPERATOR_NAME; }\
  virtual int GetTypeId(){ return OPERATOR_TYPE_ID; }\
\
  virtual int Lbp(){ return OPERATOR_LBP; }\
\
  virtual Value *Led(Value *left);\
\
protected:\
  Operator##OPERATOR_NAME(TokenParser *p)\
      :\
    Token(0,p)\
      {}\
private:\
  Operator##OPERATOR_NAME();\
  Operator##OPERATOR_NAME(const OperatorAdd &);\
  Operator##OPERATOR_NAME &operator=(const OperatorAdd &);\
};\
\
/*-----------------------------------------------------------------------------*/\
Value *Operator##OPERATOR_NAME::Led(Value *left)\
{\
  Value *right=this->GetTokenParser()->Parse(this->Lbp());\
  return left->OPERATOR_NAME(right);\
}

InfixOperator(Add,ADD_TYPE,ADD_LBP)
InfixOperator(Multiply,MULTIPLY_TYPE,MULTIPLY_LBP)
InfixOperator(Divide,DIVIDE_TYPE,DIVIDE_LBP)



/*


//=============================================================================
class OperatorAdd : public Token
{
public:
  static OperatorAdd *New(TokenParser *p){ return new OperatorAdd(p); }

  virtual const char *GetTypeString(){ return "OperatorAdd"; }
  virtual int GetTypeId(){ return ADD_TYPE; }

  virtual int Lbp(){ return ADD_LBP; }

  //virtual Value *Nud(){ return this->GetTokenParser()->Parse(this->Traits.Lbp()); }
  virtual Value *Led(Value *left);

protected:
  OperatorAdd(TokenParser *p)
      :
    Token(0,p)
      {}
private:
  OperatorAdd();
  OperatorAdd(const OperatorAdd &);
  OperatorAdd &operator=(const OperatorAdd &);
};

//-----------------------------------------------------------------------------
Value *OperatorAdd::Led(Value *left)
{
  Value *right=this->GetTokenParser()->Parse(this->Lbp());
  return left->Add(right);
}

//=============================================================================
class OperatorMultiply : public Token
{
public:
  static OperatorMultiply *New(TokenParser *p){ return new OperatorMultiply(p); }

  virtual const char *GetTypeString(){ return "OperatorMultiply"; }
  virtual int GetTypeId(){ return MULTIPLY_TYPE; }

  virtual int Lbp(){ return MULTIPLY_LBP; }

  virtual Value *Led(Value *left);

protected:
  OperatorMultiply(TokenParser *p)
      :
    Token(0,p)
      {}
private:
  OperatorMultiply();
  OperatorMultiply(const OperatorMultiply &);
  OperatorMultiply &operator=(const OperatorMultiply &);
};

//-----------------------------------------------------------------------------
Value *OperatorMultiply::Led(Value *left)
{
  Value *right=this->GetTokenParser()->Parse(this->Lbp());
  return left->Multiply(right);
}*/




int main(int argc, char **argv)
{
  TokenParser P;
  P.Push(Operand::New(3,&P));
  P.Push(OperatorAdd::New(&P));
  P.Push(Operand::New(4,&P));
  P.Push(OperatorMultiply::New(&P));
  P.Push(Operand::New(7,&P));
  P.Push(OperatorAdd::New(&P));
  P.Push(Operand::New(11,&P));
  P.Push(OperatorMultiply::New(&P));
  P.Push(Operand::New(13,&P));
  P.Push(Terminator::New());

  Value *res=P.Parse(0);
  cerr << "3+4*7+13*11=" << *res << endl;

  return 0;
}






//     class literal_token:
//         def __init__(self, value):
//             self.value = value
//         def nud(self):
//             return self.value
// 
//     class operator_add_token:
//         lbp = 10
//         def nud(self):
//             return expression(100)
//         def led(self, left):
//             return left + expression(10)
// 
//     class operator_sub_token:
//         lbp = 10
//         def nud(self):
//             return -expression(100)
//         def led(self, left):
//             return left - expression(10)
// 
//     class operator_mul_token:
//         lbp = 20
//         def led(self, left):
//             return left * expression(20)
// 
//     class operator_div_token:
//         lbp = 20
//         def led(self, left):
//             return left / expression(20)
// 
//     class operator_pow_token:
//         lbp = 30
//         def led(self, left):
//             return left ** expression(30-1)
// 
//     class end_token:
//         lbp = 0
// 
//     def tokenize(program):
//         for number, operator in re.findall("\s*(?:(\d+)|(\*\*|.))", program):
//             if number:
//                 yield literal_token(int(number))
//             elif operator == "+":
//                 yield operator_add_token()
//             elif operator == "-":
//                 yield operator_sub_token()
//             elif operator == "*":
//                 yield operator_mul_token()
//             elif operator == "/":
//                 yield operator_div_token()
//             elif operator == "**":
//                 yield operator_pow_token()
//             else:
//                 raise SyntaxError("unknown operator: %r" % operator)
//         yield end_token()
// 
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
// 
//     def parse(program):
//         global token, next
//         next = tokenize(program).next
//         token = next()
//         print program, "->", expression()
// 
// parse("+1")
// parse("-1")
// parse("10")
// parse("1**2**3")
// parse("1+2")
// parse("1+2+3")
// parse("1+2*3")
// parse("1*2+3")
// parse("5+2.0/3.0")
// parse("*1") # invalid syntax






/// Token specializations
// //=============================================================================
// template<typename T>
// class TokenTemplate : public Token
// {};
// 
// //=============================================================================
// template<>
// class Operand : public Token
// {
// public:
//   Operand(Value *v)
//       :
//     Token
//   virtual int GetTypeId(){ return this->Traits.TypeId(); }
//   virtual Value *Nud(){ return this->GetValue(); }
// 
// private:
//   TokenTraits<Operand> Traits;
//   
// };
// 
// //=============================================================================
// template<>
// class TokenTemplate<AddOperator> : public Token
// {
// public:
//   virtual int GetTypeId(){ return this->Traits.TypeId(); }
//   //virtual Value *Nud(){ return this->GetTokenParser()->Parse(this->Traits.Lbp()); }
//   virtual Value *Led(Value *left);
// 
// private:
//   TokenTraits<Operand> Traits;
// };
// 
// //-----------------------------------------------------------------------------
// Value *TokenTemplate<AddOperator>::Led(Value *left)
// {
//   MP_Value *right=this->GetTokenParser()->Parse(this->Traits.Lbp());
//   return left->Add(right);
// }
// 
// 
// //=============================================================================
// template<>
// class TokenTemplate<MultiplyOperator> : public Token
// {
// public:
//   virtual int GetTypeId(){ return this->Traits.TypeId(); }
//   virtual Value *Led(Value *left);
// 
// private:
//   TokenTraits<Operand> Traits;
// };
// 
// //-----------------------------------------------------------------------------
// Value *TokenTemplate<AddOperator>::Led(Value *left)
// {
//   MP_Value *right=this->GetTokenParser()->Parse(this->Traits.Lbp());
//   return left->Multiply(right);
// }


// //=============================================================================
// class Operand{};
// class AddOperator{};
// class MultipyOperator{};
// 
// //=============================================================================
// template<typename>
// class TokenTraits
// {};
// 
// #define TokenTraitsSpecialization(TOKEN_CLASS,TOKEN_TYPE,TOKEN_LBP)\
// /*===========================================================================*/\
// template<>\
// class TokenTraits<TOKEN_CLASS>\
// {\
//   const char *TypeString(){ return #TOKEN_CLASS; }\
//   int TypeId(){ return TOKEN_TYPE; }\
//   int Lbp(){ return TOKEN_LBP; }\
// };
// 
// TokenTraitsSpecialization(Operand,OPERAND_TYPE,OPERAND_LBP)
// TokenTraitsSpecialization(AddOperator,ADD_TYPE,ADD_LBP)
// TokenTraitsSpecialization(MultiplyOperator,MULTIPLY_TYPE,MULTIPLY_LBP)



