#ifndef __TESTCLASS_H__
#define __TESTCLASS_H__

#include <iostream>
#include <boost/serialization/base_object.hpp>

using namespace std;

class TestClass {

  int a; 
  float b;

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version){
    ar & a & b;
  }

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
