#include "CE_Adaptor.h"
#include "Matlab_Adaptor.h"
#include "Comp_Model.h"
#include "Master_Worker_Model.h"
#include <iostream>
#include <mpi.h>
using namespace std;

int main(int argc, char **argv) {

  int rank, size;

  MPI_Init (&argc, &argv);  
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);  
  MPI_Comm_size (MPI_COMM_WORLD, &size);  

  /* if ( matlab && master_worker ) then: */

  CE_Adaptor *ce = new Matlab_Adaptor();
  Master_Worker_Model *mw = new Master_Worker_Model(ce);

  if (rank==0)
    ce->Launch_CE(mw->get_m_cmd());

  else
    ce->Launch_CE(mw->get_w_cmd());

  delete mw;
  delete ce;

  MPI_Finalize();

  return 0;

}
