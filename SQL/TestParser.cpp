#include <iostream>
using std::cerr;
using std::endl;

#include "TokenParser.h"
#include "Tokens.h"
#include "Variants.h"



//-----------------------------------------------------------------------------
void TestArithmetic()
{
  cerr
    << endl
    << "(-3-4)*(7+11)*13/6=";

  TokenParser *parser=TokenParser::New();
  TokenList *program=parser->GetProgram();
  program->AppendNew(Paren::New(parser));              // (
  program->AppendNew(Minus::New(parser));              // -
  program->AppendNew(Operand::New(3,parser));          // 3
  program->AppendNew(Subtract::New(parser));           // -
  program->AppendNew(Operand::New(4,parser));          // 4
  program->AppendNew(ParenTerminator::New(parser));    // )
  program->AppendNew(Multiply::New(parser));           // *
  program->AppendNew(Paren::New(parser));              // (
  program->AppendNew(Operand::New(7,parser));          // 7
  program->AppendNew(Add::New(parser));                // +
  program->AppendNew(Operand::New(11,parser));         // 11
  program->AppendNew(ParenTerminator::New(parser));    // )
  program->AppendNew(Multiply::New(parser));           // *
  program->AppendNew(Operand::New(13,parser));         // 13
  program->AppendNew(Divide::New(parser));             // /
  program->AppendNew(Operand::New(6,parser));          // 6
  program->AppendNew(ProgramTerminator::New(parser));  // EOF

  parser->Parse();

  //cerr << *(parser->GetByteCode());
  Variant *res=parser->Eval();
  cerr << "=" << *res << endl;
  res->Delete();

  parser->Clear();
  parser->Delete();

  cerr << endl;
}

//-----------------------------------------------------------------------------
void TestLogic()
{
  cerr
    << endl
    << "3!=4->";

  TokenParser *parser=TokenParser::New();
  TokenList *program=parser->GetProgram();
  program->AppendNew(Operand::New(2,parser));
  program->AppendNew(Equal::New(parser));
  program->AppendNew(Operand::New(3,parser));
  program->AppendNew(Or::New(parser));
  program->AppendNew(Operand::New(3,parser));
  program->AppendNew(Equal::New(parser));
  program->AppendNew(Operand::New(4,parser));
  program->AppendNew(Or::New(parser));
  program->AppendNew(Operand::New(3,parser));
  program->AppendNew(Equal::New(parser));
  program->AppendNew(Operand::New(5,parser));
  program->AppendNew(ProgramTerminator::New(parser));

  parser->Parse();

  cerr << *(parser->GetByteCode()) << endl;

  parser->Clear();
  parser->Delete();

  cerr << endl;
}

//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{

  TestArithmetic();
  TestLogic();

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
//   Operand(Variant *v)
//       :
//     Token
//   virtual int GetTypeId(){ return this->Traits.TypeId(); }
//   virtual Variant *Nud(){ return this->GetVariant(); }
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
//   //virtual Variant *Nud(){ return this->GetTokenParser()->Parse(this->Traits.Lbp()); }
//   virtual Variant *Led(Variant *left);
// 
// private:
//   TokenTraits<Operand> Traits;
// };
// 
// //-----------------------------------------------------------------------------
// Variant *TokenTemplate<AddOperator>::Led(Variant *left)
// {
//   MP_Variant *right=this->GetTokenParser()->Parse(this->Traits.Lbp());
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
//   virtual Variant *Led(Variant *left);
// 
// private:
//   TokenTraits<Operand> Traits;
// };
// 
// //-----------------------------------------------------------------------------
// Variant *TokenTemplate<AddOperator>::Led(Variant *left)
// {
//   MP_Variant *right=this->GetTokenParser()->Parse(this->Traits.Lbp());
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
// #define TokenTraitsSpecialization(TOKEN_CLASS,TOKEN_TYPE,TOKEN_BP)\
// /*===========================================================================*/\
// template<>\
// class TokenTraits<TOKEN_CLASS>\
// {\
//   const char *TypeString(){ return #TOKEN_CLASS; }\
//   int TypeId(){ return TOKEN_TYPE; }\
//   int Lbp(){ return TOKEN_BP; }\
// };
// 
// TokenTraitsSpecialization(Operand,OPERAND_TYPE,OPERAND_BP)
// TokenTraitsSpecialization(AddOperator,ADD_TYPE,ADD_BP)
// TokenTraitsSpecialization(MultiplyOperator,MULTIPLY_TYPE,MULTIPLY_BP)



