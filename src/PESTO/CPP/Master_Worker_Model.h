#ifndef __MW_MODEL__
#define __MW_MODEL__

#include <iostream>
#include "CE_Adaptor.h"
using namespace std;

class Master_Worker_Model : Comp_Model {

  protected:
    char *master_cmd;
    char *worker_cmd;


  public:
    Master_Worker_Model(CE_Adaptor *);
    ~Master_Worker_Model();
    char *get_m_cmd();
    char *get_w_cmd();

};

#endif

