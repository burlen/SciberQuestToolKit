#include <iostream>
using std::cin;
using std::cerr;
using std::endl;

#include "Tokens.h"
#include "Tokenizer.h"
#include "TokenParser.h"
#include "Variants.h"

//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  while (1)
    {
    cerr << "$:";
    char buf[1024];
    cin.getline(buf,1024);
    if (cin.eof())
      {
      cerr << "EOF encountered." << endl;
      break;
      }
    size_t n=cin.gcount();
    if (n<=0)
      {
      cerr << "Error empty input." << endl;
      continue;
      }

    TokenParser *parser=TokenParser::New();
    parser->Tokenize(buf,n);
    parser->Parse();
    Variant *res=parser->Eval();

    // cerr << *(parser->GetProgram()) << endl;
    cerr << *(parser->GetByteCode()) << endl;
    cerr << *res << endl;

    res->Delete();
    parser->Clear();
    parser->Delete();
    }

  return 0;
}
