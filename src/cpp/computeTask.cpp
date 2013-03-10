/* Your includes here */
#include "TestClass.h"
#include "HmUtil.h"

/* Need these */
#include "env.hpp"
#include <iostream>
#include <string>
#include <fstream>
using namespace std;


string computeTask(string function_name, string serialized_file, string taskid) {

  cout << "function name: " << function_name << endl;

  cout << "datafile: " << serialized_file << endl;


  TestClass T;

  ifstream ifs(serialized_file.c_str(), ios::binary);

  ifs.read((char *)&T, sizeof(T));


  cout << "T.a : " << T.getA() << endl;
  cout << "T.b : " << T.getB() << endl;
  
  TestClass OutT;
  if ( function_name == "TestKernel1" ) {
    OutT.TestKernel1(&T);
  }
  else if ( function_name == "TestKernel2" ) {
    T.TestKernel2();
  }

  cout << "OutT.a : " << OutT.getA() << endl;
  cout << "OutT.b : " << OutT.getB() << endl;

  cout << "T.a : " << T.getA() << endl;
  cout << "T.b : " << T.getB() << endl;

  cout << "ext : " << HmUtil::GetFileExt(serialized_file)  << endl;



  Env env;
  string uid = env.getUid();
  cout << "uid is : " << uid << endl;

  string results_filename = "/dev/shm/." + uid + "_r" + taskid + ".mat";

  ofstream ofs(results_filename.c_str(), ios::binary);
  ofs.write((char *)&OutT, sizeof(OutT));
  ofs.close();
  
  return results_filename;

}
