#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <mpi.h>
#include <map>
#include <vector>
#include <sys/time.h>
#include "Poisson.h"
#include <sstream>
#include "LeadField.h"
#include "HmUtil.h"

#ifdef _DEBUG
#define DEBUG_PRINT(x) printf(x);
#else
#define DEBUG_PRINT(x)
#endif

using namespace std;

int main(int argc, char** argv){

  char procname[MPI_MAX_PROCESSOR_NAME];
  int namelen, rank, size;

  MPI_Init( &argc, &argv );
  MPI_Comm_size( MPI_COMM_WORLD, &size );
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Get_processor_name(procname, &namelen);

  if (size < 2 ) {
    HmUtil::ExitWithError("Must run with at least two mpi processes ");
  }

  if (rank == 0) {
    cout<<"\n                     LFM COMPUTATION                    " << endl;
    cout<<"\n==========================  Input Script =========================" << endl;
  }

  string line, param, value;
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

  if (rank == 0) 
    cout<<"\n==================================================================" << endl;

  LeadField lf;
  if (lf.Init(parameters)){
    cerr << "Error: initialization ... " << endl;
    return 1;
  }
  
  lf.Compute();
  cout << "[" << rank << "] Done" << endl;

  MPI_Finalize();

  return 0;
}
