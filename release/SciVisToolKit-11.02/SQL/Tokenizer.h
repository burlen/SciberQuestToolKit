/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef Tokenizer_h
#define Tokenizer_h


#include "RefCountedPointer.h"

class TokenParser;

class Tokenizer
{
public:
  Tokenizer()
      :
    Parser(0)
      {}

  ~Tokenizer(){ this->SetParser(0); }

  void Tokenize(const char *str, size_t n);

  SetRefCountedPointer(Parser,TokenParser);

protected:
  Tokenizer(const Tokenizer &); // not implemented
  Tokenizer &operator=(const Tokenizer &);

private:
  TokenParser *Parser;
};

#endif
