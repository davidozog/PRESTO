/*! \class ObjFunc
* \brief Abstract base class used as interface for implementing objective functions.

* \author Adnan Salman
* \author NeuroInformatic Center
* \date 2012
*/

#ifndef __OBJFUNC__
#define __OBJFUNC__

#include <vector>

class ObjFunc{
 public:
  virtual ~ObjFunc(){};
  virtual int operator()(int, std::vector<float>, float&) = 0;
};

#endif
