#ifndef __MATLAB_ADAPTOR__
#define __MATLAB_ADAPTOR__

#include <iostream>
using namespace std;

class Matlab_Adaptor : public CE_Adaptor {

  protected:
    char *name;
  
  public:

    Matlab_Adaptor();
    ~Matlab_Adaptor();
    virtual void Launch_CE(char *);
    virtual char * get_name();
    virtual char * set_name();


};

#endif

