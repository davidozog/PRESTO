#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include "cond_class.h"
#include "HmUtil.h"

using std::string;
using std::cout;
using std::endl;
using std::set;

int main(int argc, char** argv){

  int rank, namelen, size;
  char procname[MPI_MAX_PROCESSOR_NAME];
  char Cprocname[MPI_MAX_PROCESSOR_NAME];

  MPI_Init( &argc, &argv );
  MPI_Comm_size( MPI_COMM_WORLD, &size );
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Get_processor_name(procname, &namelen);

  if (rank == 0) {
    cout<<"\n                     COND COMPUTATION                    " << endl;
    cout<<"\n====================  Input Script ======================" << endl;
  }

  string line, pvalue, param;
  string value;

  map<string, vector<string> > parameters;
  ifstream ins(argv[1]);
                                                                         
  while(getline(ins, line, '\n')){
    if (line[0] == '#' || line.empty())continue;
    stringstream lines(stringstream::in | stringstream::out );
    lines << line;
    if (rank == 0) cout << line << endl;
    lines >> param;
    if (param[0] == '#') continue;

    while (lines >> value) {
      parameters[param].push_back(value);
    }
  }
   
  if (rank == 0) {
    cout<<"\n=========================================================" << endl;
    cout << endl;
  } 

  string  measured_file_name = ""; 
  string datapath = "./";
  if (parameters.find("measured_data") !=  parameters.end())
    measured_file_name = parameters["measured_data"][0];

  CondInv inv(parameters, rank);
  inv.Optimize(measured_file_name);

  MPI_Finalize();
}


