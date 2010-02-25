
#include <iostream>
using std::ostream;
using std::cerr;
using std::endl;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include "Values.h"


// removes spaces,tabs and newlines in place.
//*****************************************************************************
void StripWhiteSpace(string &str)
{
//   size_t n=str.size();
// 
//   char *pOut=str.c_str();
//   char *pIn=str.c_str();
//   char *pInEnd=pIn+n;
// 
//   for (; pIn<pInEnd; ++pIn)
//     {
//     if ((*pIn==' ')||(*pIn=='\t')||(*pIn=='\n'))
//       {
//       continue;
//       }
//     *pOut=*pIn;
//     ++pOut;
//     }
// 
//   n=pOut-pIn+1;
//   str.resize(n);
}

// replaces the array name (delimitted by @) with an integer index
// that will be used to access the array data.
//*****************************************************************************
void ArrayNameToIndex(string &in, vector<string> &arrayNames)
{
//   size_t n=arrayNames.size();
//   for (size_t i=0; i<n; ++i)
//     {
//     string arrayName;
//     arrayName="@"+arrayNames[i]+"@";
//     ostringstream os;
//     os << i;
//     SearchAndReplace(arrayName,os.str().c_str(),in);
//     }
}









template <typename T>
class LogicalExpression
{
public:
  void SetExpression();
  bool Eval(int idx);
  enum{
    CONSTANT,
    ADD,
    SUB,
    MUL,
    DIV,
    NEG,
    ABS,
    AND,
    OR,
    NOT,
    EQUAL,
    LESS,
    GREATER,
    LESS_EQUAL,
    GREATER_EQUAL
    };
private:
  vector<int> ByteCode;
  T *Result;
  vector<T*> Arrays;
  vector<int> Comps;
  vector<double> Constants;
};



int main(int argc, char **argv)
{
//   if (argc!=2)
//     {
//     cerr << "Error: " << argv[0] << " expression" << endl;
//     }

  // mixed type operation.
  Value *dv=new DoubleValue(1.5678);
  Value *iv=new IntValue(2);

  cerr << *dv << " < " << *iv << " = " << *(dv->Less(iv)) << endl;

  // array type operation
  int na=3;
  double a[3]={-1.0,1.56,1000.0};
  Value *dvp=new DoubleValuePointer(a);

  for (int i=0; i<na; ++i)
    {
    cerr << *dv << " < " << *dvp << " = " << *(dv->Less(dvp)) << endl;
    dvp->Next();
    }

  return 0;
}
