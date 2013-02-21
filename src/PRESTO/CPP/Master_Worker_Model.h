#ifndef __MW_MODEL__
#define __MW_MODEL__

#include <iostream>
#include "CE_Adaptor.h"
using namespace std;

class Master_Worker_Model : Comp_Model {

  protected:
    const char *master_cmd;
    char *worker_cmd;
    const char *hostname;

  public:
    Master_Worker_Model(CE_Adaptor *, char *, int);
    ~Master_Worker_Model();
    const char *get_m_cmd();
    char *get_w_cmd();
    const char *get_hostname();

};

#endif

