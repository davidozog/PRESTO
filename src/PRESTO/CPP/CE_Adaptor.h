#ifndef __CE_ADAPTOR__
#define __CE_ADAPTOR__

#include <iostream>
using namespace std;

class CE_Adaptor {

  protected:
  
  public:

    CE_Adaptor();
    virtual ~CE_Adaptor();
    virtual void Launch_CE(const char *, int)=0;
    virtual char * get_name()=0;
    virtual char * set_name()=0;

};

#endif
