#ifndef __TESTCLASS_H__
#define __TESTCLASS_H__

#include <iostream>
using namespace std;

class TestClass {

  private:
    int a; 
    float b;

  public:
    TestClass();
    TestClass(int, float);
    ~TestClass();
    int getA();
    float getB();
    void setA(int);
    void setB(float);
    TestClass * TestKernel1(TestClass *);
    TestClass * TestKernel2();

};

#endif
