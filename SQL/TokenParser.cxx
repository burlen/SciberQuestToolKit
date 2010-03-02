/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/

#include "TokenParser.h"

#include "Token.h"
#include "TokenList.h"
#include "Variant.h"
#include "VariantStack.h"

//-----------------------------------------------------------------------------
TokenParser::TokenParser()
    :
  Program(0),
  ByteCode(0)
{
  this->SetNewProgram(TokenList::New());
  this->SetNewByteCode(TokenList::New());
}

//-----------------------------------------------------------------------------
TokenParser::~TokenParser()
{
  this->SetProgram(0);
  this->SetByteCode(0);
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
  VariantStack *Stack=VariantStack::New();

  this->ByteCode->IteratorBegin();
  while (this->ByteCode->IteratorOk())
    {
    Token *t=this->ByteCode->GetCurrent();
    t->Operate(Stack);

    this->ByteCode->IteratorIncrement();
    }

  Variant *res=Stack->Pop();
  res->Register();

  Stack->Clear();
  Stack->Delete();

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
