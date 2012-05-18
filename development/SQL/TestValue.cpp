#include <iostream>
using std::ostream;
using std::cerr;
using std::endl;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include "Values.h"




void TestIterator(Value *v)
{
  cerr << "  " << v->GetTypeString();
  v->IteratorBegin();
  do
    {
    cerr << " " << *v;
    v->IteratorAdvance();
    }
  while (v->IteratorOk());
  cerr << endl;
}

void TestAddAssign(Value *v)
{
  cerr << "  " << v->GetTypeString() << " Sum(v) ";
  MP_Value r=v->NewInstance();
  v->IteratorBegin();
  do
    {
    r->AddAssign(v);
    v->IteratorAdvance();
    }
  while (v->IteratorOk());
  cerr << *r;
  cerr << endl;
}

void TestSubtractAssign(Value *v)
{
  cerr << "  " << v->GetTypeString() << " -Sum(v) ";
  MP_Value r=v->NewInstance();
  v->IteratorBegin();
  do
    {
    r->SubtractAssign(v);
    v->IteratorAdvance();
    }
  while (v->IteratorOk());
  cerr << *r;
  cerr << endl;
}

void TestL2Norm(Value *v)
{
  cerr << "  " << v->GetTypeString() << " L2 Norm" << endl;
  v->IteratorBegin();
  do
    {
    MP_Value n=v->L2Norm();
    cerr << "  |" <<  *v  << "|=" << *n << endl;
    v->IteratorAdvance();
    }
  while (v->IteratorOk());
  cerr << endl;
}


#define TEST_AB(CPP_OP,METHOD_NAME,A,B)\
cerr << "  " <<  *A << #CPP_OP << *B << " " << *(A->METHOD_NAME(B)) << endl;

#define TEST_ABC(CPP_OP,METHOD_NAME,A,B,C)\
TEST_AB(CPP_OP,METHOD_NAME,A,B)\
TEST_AB(CPP_OP,METHOD_NAME,B,C)

#define TEST(CPP_OP,METHOD_NAME)\
cerr << "Testing Value::" #METHOD_NAME << endl;\
TEST_ABC(CPP_OP,METHOD_NAME,cv0,cv1,cv2)\
TEST_ABC(CPP_OP,METHOD_NAME,bv0,bv1,bv2)\
TEST_ABC(CPP_OP,METHOD_NAME,iv0,iv1,iv2)\
TEST_ABC(CPP_OP,METHOD_NAME,fv0,fv1,fv2)\
TEST_ABC(CPP_OP,METHOD_NAME,dv0,dv1,dv2)\
TEST_ABC(CPP_OP,METHOD_NAME,llv0,llv1,llv2)\
TEST_ABC(CPP_OP,METHOD_NAME,cv0,cv1,cv2)\
TEST_AB(CPP_OP,METHOD_NAME,fv0,dv0)\
TEST_AB(CPP_OP,METHOD_NAME,fv0,iv0)\
TEST_AB(CPP_OP,METHOD_NAME,fv0,llv0)\


int main(int argc, char **argv)
{
  cerr << "Test suite for Value.h" << endl;


  const int N=4;

  // for each type create 2 constant, 1 array, and 1 vector.
  MP_Value cv0=CharValue::New('a');
  MP_Value cv1=CharValue::New('b');
  MP_Value cv2=CharValue::New('b');
  char ca[N]={'a','b','c','d'};
  MP_Value cvp=CharValuePointer::New(ca,4);
  MP_Value cvv=CharValuePointer::New(ca,2,2);

  MP_Value bv0=BoolValue::New(false);
  MP_Value bv1=BoolValue::New(true);
  MP_Value bv2=BoolValue::New(true);
  bool ba[N]={1,0,0,1};
  MP_Value bvp=BoolValuePointer::New(ba,4);
  MP_Value bvv=BoolValuePointer::New(ba,2,2);

  MP_Value iv0=IntValue::New(2);
  MP_Value iv1=IntValue::New(3);
  MP_Value iv2=IntValue::New(3);
  int ia[N]={-1,1,-2,2};
  MP_Value ivp=IntValuePointer::New(ia,4);
  MP_Value ivv=IntValuePointer::New(ia,2,2);

  MP_Value fv0=FloatValue::New(2);
  MP_Value fv1=FloatValue::New(3);
  MP_Value fv2=FloatValue::New(3);
  float fa[N]={-1.1,1.1,-2.2,2.2};
  MP_Value fvp=FloatValuePointer::New(fa,4);
  MP_Value fvv=FloatValuePointer::New(fa,2,2);

  MP_Value dv0=DoubleValue::New(2);
  MP_Value dv1=DoubleValue::New(3);
  MP_Value dv2=DoubleValue::New(3);
  double da[N]={-1.1,1.1,-2.2,2.2};
  MP_Value dvp=DoubleValuePointer::New(da,4);
  MP_Value dvv=DoubleValuePointer::New(da,2,2);

  MP_Value llv0=LongLongValue::New(2);
  MP_Value llv1=LongLongValue::New(3);
  MP_Value llv2=LongLongValue::New(3);
  long long lla[N]={-1,1,-2,2};
  MP_Value llvp=LongLongValuePointer::New(lla,4);
  MP_Value llvv=LongLongValuePointer::New(lla,2,2);


  // Logical (return the result in a new Value)
  cerr << "Testing Value::Next" << endl;
  TestIterator(cvp);
  TestIterator(cvv);
  TestIterator(bvp);
  TestIterator(bvv);
  TestIterator(ivp);
  TestIterator(ivv);
  TestIterator(fvp);
  TestIterator(fvv);
  TestIterator(dvp);
  TestIterator(dvv);
  TestIterator(llvp);
  TestIterator(llvv);

  // Arithmetic (return the result in a new Value)
  TEST(==,Equal)
  TEST(==,NotEqual)
  TEST(<,Less)
  TEST(<=,LessEqual)
  TEST(>,Greater)
  TEST(>=,GreaterEqual)
  TEST(+,Add)
  TEST(-,Subtract)
  TEST(*,Multiply)
  TEST(/,Divide)

  // NOTE vectors don't work
  cerr << "Testing Value::AddAssign" << endl;
  TestAddAssign(cvp);
  TestAddAssign(bvp);
  TestAddAssign(ivp);
  TestAddAssign(fvp);
  TestAddAssign(dvp);
  TestAddAssign(llvp);

  // NOTE vectors don't work
  cerr << "Testing Value::SubtractAssign" << endl;
  TestSubtractAssign(cvp);
  TestSubtractAssign(bvp);
  TestSubtractAssign(ivp);
  TestSubtractAssign(fvp);
  TestSubtractAssign(dvp);
  TestSubtractAssign(llvp);

  cerr << "Testing Value::L2Norm" << endl;
  TestL2Norm(cvp);
  TestL2Norm(cvv);
  TestL2Norm(bvp);
  TestL2Norm(bvv);
  TestL2Norm(ivp);
  TestL2Norm(ivv);
  TestL2Norm(fvp);
  TestL2Norm(fvv);
  TestL2Norm(dvp);
  TestL2Norm(dvv);
  TestL2Norm(llvp);
  TestL2Norm(llvv);


  cerr << "Testing Value::Or" << endl;
  ManagedPointer<Value> res1,res2,res3;
  res1=iv0->Equal(iv1);
  res2=iv1->Equal(iv2);
  res3=res1->Or(res2);
  cerr <<  *iv0 << "==" << *iv1 << "||" << *iv1 << "==" << *iv2 << " " << *res3 << endl;

//   cerr << "Testing Value::And" << endl;
  
// 
//   cerr << *cv0 << ">" << "c||" << *cv1 << ">b" << *cv0->Greater(new CharValue('c'))->Or(cv1->GreaterEqual(new CharValue('b'))) << endl;
    

// TODO
//   cerr << "Testing Value::And" << endl;
    
//   cerr << "Testing Value::Not" << endl;
// 
// 
//   cerr << "Testing Value::Sqrt" << endl;
//   cerr << "Testing Value::Negate" << endl;
//   // In-place arithmetic (return pointer to this)
//   cerr << "Testing Value::AddAssign" << endl;
//   cerr << "Testing Value::SubtractAssign" << endl;
//   cerr << "Testing Value::MultiplyAssign" << endl;
//   cerr << "Testing Value::DivideAssign" << endl;
//   cerr << "Testing Value::Assign" << endl;
// 
//   // Vector arirthmetic (return result in a new Value)
//   cerr << "Testing Value::Component" << endl;



  return 0;
}
