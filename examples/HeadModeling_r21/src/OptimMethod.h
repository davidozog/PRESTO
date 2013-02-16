#ifndef __OPTIM_METHOO___
#define __OPTIM_METHOO___

/*! \class OptimMethod
* \brief Abstract base class used as interface for implementing optimization  methods, any optimization method must implemnt the abstract methods.

* \author Adnan Salman
* \author NeuroInformatic Center
* \date 2012
*/

#include <iostream>
#include <vector>
#include <stdlib.h>
#include "ObjFunc.h"
#include <map>

using std::vector;
using std::ostream;
using std::map;
using std::string;

class OptimMethod  {

 protected:

  int             mNumberOfVariables; //!< Number of variable parameters              
  int             mMaxFcnEval;        //!< Maximum objective function evaluation              

  vector<float>   mVarLowerBound;     //!< Lower bound on the search 
  vector<float>   mVarUpperBound;     //!< Upper bound on the search 

  float           mTolerance;         //!< Terminates if objective function is below this value
  ObjFunc *       mpObjFunc;          //!< Objective function to use see CondOptmOF class for choices 
  int             mNumberFcnEval;     //!< Number of obj function evaluation  

 public:
  
  virtual ~OptimMethod();

  void SetMaxFcnEval(int maxFcnEval);
  void SetObjFunc(ObjFunc *pObjFunc);
  void SetVarLowerBound(vector<float> varLowerBound);
  void SetVarUpperBound(vector<float> varUpperBound);
  void SetTolerance(float tolerance);

  float RandUniform01();
  float RandUniform(float x1, float x2);
  int GetNumberFcnEval();

  virtual int  Optimize(vector<float> &rInitVariables, float& rOptimObjFunc, 
			ObjFunc *pObjFunc, ostream* pouts) = 0;
  virtual void PrintInfo(ostream& rOuts = std::cout) = 0;
  virtual void SetParams(map<string, vector<string> > inputParams);

};

#endif
