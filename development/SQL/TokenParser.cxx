/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/

#include "TokenParser.h"

#include <cstdlib>

#include "Token.h"
#include "Tokens.h"
#include "TokenList.h"
#include "Variant.h"
#include "VariantStack.h"

//-----------------------------------------------------------------------------
TokenParser::TokenParser()
    :
  Program(0),
  ByteCode(0),
  Stack(0)
{
  this->SetNewProgram(TokenList::New());
  this->SetNewByteCode(TokenList::New());
  this->SetNewStack(VariantStack::New());
}

//-----------------------------------------------------------------------------
TokenParser::~TokenParser()
{
  this->SetProgram(0);
  this->SetByteCode(0);
  this->SetStack(0);
}

//-----------------------------------------------------------------------------
void TokenParser::Clear()
{
  this->Program->Clear();
  this->ByteCode->Clear();
}

//-----------------------------------------------------------------------------
SetRefCountedPointerImpl(TokenParser,Program,TokenList);

//-----------------------------------------------------------------------------
SetRefCountedPointerImpl(TokenParser,ByteCode,TokenList);

//-----------------------------------------------------------------------------
SetRefCountedPointerImpl(TokenParser,Stack,VariantStack);

//-----------------------------------------------------------------------------
void TokenParser::Tokenize(const char *str, size_t n)
{
  char *end;
  double value;

  size_t i=0;
  while (i<n)
    {
    /// White space
    while ((str[i]==' ')||(str[i]=='\n')||(str[i]=='\t')){ ++i; }

    /// Operators
    switch (str[i])
      {
      case '\0':
        this->Program->AppendNew(ProgramTerminator::New(this));
        return;
        break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        value=strtod(&str[i],&end);
        this->Program->AppendNew(Operand::New(value,this));
        i+=end-&str[i];
        continue;
        break;

      case '@':
        // TODO arrays
        continue;
        break;

      case '(':
        this->Program->AppendNew(Paren::New(this));
        ++i;
        continue;
        break;

      case ')':
        this->Program->AppendNew(ParenTerminator::New(this));
        ++i;
        continue;
        break;

      case '[':
        this->Program->AppendNew(Bracket::New(this));
        ++i;
        continue;
        break;

      case ']':
        this->Program->AppendNew(BracketTerminator::New(this));
        ++i;
        continue;
        break;

      case '+':
        this->Program->AppendNew(Add::New(this));
        ++i;
        continue;
        break;

      case '-':
        this->Program->AppendNew(Minus::New(this));
        ++i;
        continue;
        break;

      case '*':
        this->Program->AppendNew(Multiply::New(this));
        ++i;
        continue;
        break;

      case '/':
        this->Program->AppendNew(Divide::New(this));
        ++i;
        continue;
        break;

      case '&':
        if ((i+1<n)&&(str[i+1]=='&'))
          {
          this->Program->AppendNew(And::New(this));
          i+=2;
          }
        else
          {
          SyntaxError e(str,i);
          throw e;
          }
        continue;
        break;

      case '|':
        if ((i+1<n)&&(str[i+1]=='|'))
          {
          this->Program->AppendNew(Or::New(this));
          i+=2;
          }
        else
          {
          SyntaxError e(str,i);
          throw e;
          }
        continue;
        break;

      case '=':
        if ((i+1<n)&&(str[i+1]=='='))
          {
          this->Program->AppendNew(Equal::New(this));
          i+=2;
          }
        else
          {
          SyntaxError e(str,i);
          throw e;
          }
        continue;
        break;

      case '!':
        if ((i+1<n)&&(str[i+1]=='='))
          {
          this->Program->AppendNew(NotEqual::New(this));
          i+=2;
          }
        else
          {
          this->Program->AppendNew(Not::New(this));
          ++i;
          }
        continue;
        break;

      case '<':
        if ((i+1<n)&&(str[i+1]=='='))
          {
          this->Program->AppendNew(LessEqual::New(this));
          i+=2;
          }
        else
          {
          this->Program->AppendNew(Less::New(this));
          ++i;
          }
        continue;
        break;

      case '>':
        if ((i+1<n)&&(str[i+1]=='='))
          {
          this->Program->AppendNew(GreaterEqual::New(this));
          i+=2;
          }
        else
          {
          this->Program->AppendNew(Greater::New(this) );
          ++i;
          }
        continue;
        break;
      }
    /// TODO key words

    /// All other sequences are erronious
    SyntaxError e(str,i);
    throw e;
    }
}

//-----------------------------------------------------------------------------
void TokenParser::Parse()
{
  if (!this->Program)
    {
    cerr << __LINE__ << " Error no program." << endl;
    return;
    }

  this->ByteCode->Clear();

  this->Parse(0);
}

// This is the top-down recursive decent parser, deceptively
// simple.
//-----------------------------------------------------------------------------
void TokenParser::Parse(int rbp)
{
  TokenList *program=this->GetProgram();

  Token *current;

  current=program->GetCurrent();
  program->IteratorIncrement();

  current->Nud();

  while (rbp<program->GetCurrent()->Lbp())
    {
    current=program->GetCurrent();
    program->IteratorIncrement();

    current->Led();
    }
}

//-----------------------------------------------------------------------------
Variant *TokenParser::Eval()
{
  this->ByteCode->IteratorBegin();
  while (this->ByteCode->IteratorOk())
    {
    Token *t=this->ByteCode->GetCurrent();
    t->Operate(this->Stack);

    this->ByteCode->IteratorIncrement();
    }

  Variant *res=this->Stack->Pop();
  res->Register();

  this->Stack->Clear();

  return res;
}

//-----------------------------------------------------------------------------
void TokenParser::Print(ostream &os)
{
  os
    << "TokenParser : ";
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
