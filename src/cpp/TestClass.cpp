#include <iostream>
#include "TestClass.h"
using namespace std;

TestClass::TestClass() {
}

TestClass::TestClass(int i, float j) {
  this->a = i;
  this->b = j;
}

TestClass::~TestClass() {
}

int TestClass::getA() {
  return this->a;
}

float TestClass::getB() {
  return this->b;
}

void TestClass::setA(int i) {
  this->a = i;
}

void TestClass::setB(float j) {
  this->b = j;
}

TestClass * TestClass::TestKernel1(TestClass *T1) {
  this->a = 7*(T1->getA());
  this->b = 7*(T1->getB());
  return this;
}

TestClass * TestClass::TestKernel2() {
  this->a = 7*(this->a);
  this->b = 7*(this->b);
  return this;
}
