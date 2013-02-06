#include "Comp_Model.h"
#include "Master_Worker_Model.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;


Master_Worker_Model::Master_Worker_Model(CE_Adaptor *CE, char *hostnm, int namelen): Comp_Model() {

  char *ce_name;
  ce_name = CE->get_name();

  if ( strcmp(ce_name, "matlab") == 0 )  {
    master_cmd = (const char *)malloc(128);
    worker_cmd = (char *)malloc(128);
    hostname  =  (const char *)malloc(namelen*sizeof(char)+1);
    strncpy((char *)hostname, hostnm, sizeof(char)*(namelen+1));
    master_cmd = "/bin/matlab -nodesktop -nosplash";
    strcpy(worker_cmd, "/bin/matlab -nodesktop -nosplash -r \"mworker(\'");
    strcat(worker_cmd, hostnm);
    strcat((char *)worker_cmd, "\', 1)\" 2>&1 | tee mylog");
  }

}


Master_Worker_Model::~Master_Worker_Model(){

  // delete instance;
  free((void *)master_cmd);
  free((void *)worker_cmd);
  free((void *)hostname);

}

const char * Master_Worker_Model::get_m_cmd(){

  return master_cmd;
  
}

char * Master_Worker_Model::get_w_cmd(){

  return worker_cmd;
  
}

const char * Master_Worker_Model::get_hostname(){

  return hostname;
  
}

