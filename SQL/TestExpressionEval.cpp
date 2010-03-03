
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
  size_t n=str.size();
  int i_in=0;
  int i_out=0;

  for (; i_in<n; ++i_in)
    {
    char c=str[i_in];
    if ((c==' ')||(c=='\t')||(c=='\n'))
      {
      continue;
      }
    str[i_out]=c;
    ++i_out;
    }
  str.resize(i_out);
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

inline
bool IsDigit(char c)
{
  return (d>='0')&&(d<='9');
}

inline
bool IsPlus(char c)
{
  return c=='+';
}

inline
bool IsMinus(char c)
{
  return c=='-';
}

inline 
bool IsExp(char c)
{
  return (c=='e')||(c=='E');
}

inline 
bool IsPoint(char c)
{
  return c=='.';
}

int NumberOfDigits(string &str, int i)
{
  int n=str.size();
  int n_dig=0;
  while(1)
    {
    int j=i+1>=n?i:j;
    char c=str[i];
    char d=str[j];
    if (IsExp(c)&&(IsDigit(d)||IsPlus(d)||IsMinus(d)))
      {
      ++n_dig;
      ++i;
      continue;
      }
    else
    if (IsDigit(c)||(IsPoint(c)&&IsDigit(d)))
      {
      ++n_dig;
      ++i;
      continue;
      }
    break;
    }
}

//-----------------------------------------------------------------------------
int AsciiToValue(const char *str,vector<MP_Value> &vals)
{
  char *end;
  errno = 0;
  double d=strtod(str,&end,0);

  if ((errno==ERANGE)||(errno==EINVAL))
    {
    cerr << __LINE__ << " Range error." << endl;
    return 0;
    }

  if (end==str)
    {
    cerr << __LINE__ << " No number found." << endl;
    return 0;
    }

  vals.push_back(DoubleValue::New(d));

  return e-s;
}

//-----------------------------------------------------------------------------
int ParseConstants(string &str, vector<MP_Value> &vals)
{
  size_t n=str.size();

  int i_in=0;
  int i_out=0;

  int val_idx=0;

  for (; i_in<n; ++i_in)
    {
    char c=str[i_in];
    if (IsDigit(c)||IsPoint(c))
      {
      const char *pStr=str.c_str()+i_in;
      int dig_len=AsciiToValue(pStr,vals);
      if (dig_len==0)
        {
        cerr << __LINE__ << "Syntax error." << endl;
        return;
        }
      str[i_in]=val_idx;
      continue;
      }
    str[i_out]=c;
    ++i_out;
    }
  str.resize(i_out);
}

//-----------------------------------------------------------------------------
int ParseInfix(string input, vector<int> &rep)
{
  


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
