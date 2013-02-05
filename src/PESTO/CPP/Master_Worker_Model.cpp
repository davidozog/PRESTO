#include "Comp_Model.h"
#include "Master_Worker_Model.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;


Master_Worker_Model::Master_Worker_Model(CE_Adaptor *CE): Comp_Model() {

  char *ce_name;
  ce_name = CE->get_name();

  if ( strcmp(ce_name, "matlab") == 0 )  {
    master_cmd = (char *)malloc(64);
    worker_cmd = (char *)malloc(64);
    master_cmd = (char *)"/bin/matlab -nodesktop -nosplash";
    worker_cmd = (char *)"/bin/matlab -nodesktop -nosplash -r \"mworker(\'cn130\', 1)\"";
  }

  //free(ce_name);
  
}


Master_Worker_Model::~Master_Worker_Model(){

  // delete instance;
  free(master_cmd);
  free(worker_cmd);

}

char * Master_Worker_Model::get_m_cmd(){

  return master_cmd;
  
}

char * Master_Worker_Model::get_w_cmd(){

  return worker_cmd;
  
}

