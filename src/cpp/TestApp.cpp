#include <iostream>
#include "Master.h"
#include "TestClass.h"
using namespace std;


int main( int argc, char **argv ) {

  const unsigned numTasks = 3;
  cout << "hi dave" << endl;

  Master<TestClass, numTasks> M;

  TestClass *TArrayIn = new TestClass[numTasks];
  TestClass *TArrayOut = new TestClass[numTasks];

  for (int i=0; i<numTasks; i++) {
    TArrayIn[i].setA(i);
    TArrayIn[i].setB((float)(i+numTasks));
    cout << TArrayIn[i].getA() << " " << TArrayIn[i].getB() << endl;
  }
    
  M.Launch();

  M.SendJobsToWorkers("TestKernel", TArrayIn, TArrayOut);
  cout << "hunky dory" << endl;

  for (int i=0; i<numTasks; i++) {
    cout << TArrayOut[i].getA() << " " << TArrayOut[i].getB() << endl;
  }

/*

    for (int i=0; i<numTasks; i++) {
      System.out.println("Final Results #"+Integer.toString(i) +" are: " + 
      Integer.toString(TArrayOut[i].a) + " " + Float.toString(TArrayOut[i].b));
    }

*/

  M.Destroy();

  delete[] TArrayIn;
  cout << "ok..." << endl;
  delete[] TArrayOut;
    
  }

