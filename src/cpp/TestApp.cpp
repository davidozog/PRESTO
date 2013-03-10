#include <iostream>
#include "Master.h"
#include "TestClass.h"
using namespace std;


int main( int argc, char **argv ) {

  const unsigned numTasks = 3;

  Master<TestClass, numTasks> M;

  TestClass *TArrayIn = new TestClass[numTasks];
  TestClass *TArrayOut = new TestClass[numTasks];

  cout << "Input:" << endl;
  for (int i=0; i<numTasks; i++) {
    TArrayIn[i].setA(i);
    TArrayIn[i].setB((float)(i+numTasks));
    cout << "\t" << TArrayIn[i].getA() << " " << TArrayIn[i].getB() << endl;
  }
  cout << endl;
    
  M.Launch();

  M.SendJobsToWorkers("TestKernel1", TArrayIn, TArrayOut);

  cout << "Output:" << endl;
  for (int i=0; i<numTasks; i++) {
    cout << "\t" << TArrayOut[i].getA() << " " << TArrayOut[i].getB() << endl;
  }
  cout << endl;


  M.Destroy();

  delete[] TArrayIn;
  delete[] TArrayOut;
    
}
