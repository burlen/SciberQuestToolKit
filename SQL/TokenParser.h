/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef TokenParser_h
#define TokenParser_h

#include <iostream>
using std::ostream;
#include <deque>
using std::deque;
#include <exception>
using std::exception;

#include "RefCountedPointer.h"

//=============================================================================
class SyntaxError : public exception
{
  virtual const char* what() const throw()
    {
    return "Syntax Error";
    }
};

class Token;
class TokenList;
class Variant;
//=============================================================================
class TokenParser : public RefCountedPointer
{
public:
  static TokenParser *New(){ return new TokenParser; }

  // Description:
  // Access to the program to parse, this should be filled
  // with tokens prior to calling Parse(). After parsing the 
  // progrma may be Clear()'d.
  TokenList *GetProgram(){ return this->Program; }

  // Description:
  // Access to the bye code generated during Parse(). After
  // Eval() the byte code may be Clear()'d.
  TokenList *GetByteCode(){ return this->ByteCode; }

  // Description:
  // Parse the program (see GetProgram()) and generate byte code 
  // which can evaluated by calling Eval().
  void Parse();

  // Description:
  // Evaluate the current bytecode.You must have previously called
  // Parse on a string of tokens.
  Variant *Eval();

  // Description:
  // Print the object's state.
  virtual void Print(ostream &os);

  // Description:
  // Clears the program and byte code associated with the parser.
  // The tokens therein each hold a reference to the parser so this
  // MUST be called before the parser can be deleted.
  void Clear();

  ///------------------------------------------------
  // TODO encapsulate, these shouldn't be public. but Tokens need access.
  // Description:
  // Parse an expression into a byte code sequence.
  void Parse(int rbp);

protected:
  TokenParser();
  virtual ~TokenParser();

  // Description:
  // Set the program container.
  SetRefCountedPointer(Program,TokenList);

  // Description:
  // Set the byte code container.
  SetRefCountedPointer(ByteCode,TokenList);

private:
  TokenParser(const TokenParser &); // not implemented
  TokenParser &operator=(const TokenParser &);

private:
  TokenList *Program;
  TokenList *ByteCode;
};

#endif
