/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/

#include "Tokenizer.h"

#include <cstdlib>
#include "errno.h"

#include "Variants.h"
#include "Tokens.h"
#include "TokenList.h"
#include "TokenParser.h"

//-----------------------------------------------------------------------------
SetRefCountedPointerImpl(Tokenizer,Parser,TokenParser);

//-----------------------------------------------------------------------------
void Tokenizer::Tokenize(const char *str, size_t n)
{
  TokenList *program=this->Parser->GetProgram();

  size_t i=0;
  while (i<n)
    {
    /// White space
    while ((str[i]==' ')||(str[i]=='\n')||(str[i]=='\t')){ ++i; }

    /// Operators
    switch (str[i])
      {
      case '\0':
        program->AppendNew(ProgramTerminator::New(this->Parser));
        return;
        break;

      case '@':
        // TODO arrays
        continue;
        break;

      case '(':
        program->AppendNew(Paren::New(this->Parser));
        ++i;
        continue;
        break;

      case ')':
        program->AppendNew(ParenTerminator::New(this->Parser));
        ++i;
        continue;
        break;

      case '[':
        program->AppendNew(Bracket::New(this->Parser));
        ++i;
        continue;
        break;

      case ']':
        program->AppendNew(BracketTerminator::New(this->Parser));
        ++i;
        continue;
        break;

      case '+':
        program->AppendNew(Add::New(this->Parser));
        ++i;
        continue;
        break;

      case '-':
        program->AppendNew(Minus::New(this->Parser));
        ++i;
        continue;
        break;

      case '*':
        program->AppendNew(Multiply::New(this->Parser));
        ++i;
        continue;
        break;

      case '/':
        program->AppendNew(Divide::New(this->Parser));
        ++i;
        continue;
        break;

      case '&':
        if ((i+1<n)&&(str[i+1]=='&'))
          {
          program->AppendNew(And::New(this->Parser));
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
          program->AppendNew(Or::New(this->Parser));
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
          program->AppendNew(Equal::New(this->Parser));
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
          program->AppendNew(NotEqual::New(this->Parser));
          i+=2;
          }
        else
          {
          program->AppendNew(Not::New(this->Parser));
          ++i;
          }
        continue;
        break;

      case '<':
        if ((i+1<n)&&(str[i+1]=='='))
          {
          program->AppendNew(LessEqual::New(this->Parser));
          i+=2;
          }
        else
          {
          program->AppendNew(Less::New(this->Parser));
          ++i;
          }
        continue;
        break;

      case '>':
        if ((i+1<n)&&(str[i+1]=='='))
          {
          program->AppendNew(GreaterEqual::New(this->Parser));
          i+=2;
          }
        else
          {
          program->AppendNew(Greater::New(this->Parser) );
          ++i;
          }
        continue;
        break;
      }
    /// TODO key words

    /// Constants
    if ((str[i]>='0')&&(str[i]<='9')||(str[i]=='.'))
      {
      char *end;
      errno=0;
      double d=strtod(&str[i],&end);
      if ((errno==ERANGE)||(errno==EINVAL))
        {
        SyntaxError e;
        throw e;
        }
      program->AppendNew(Operand::New(d,this->Parser));
      i+=end-&str[i];
      }
    else
      {
      SyntaxError e(str,i);
      throw e;
      }

    }
}




