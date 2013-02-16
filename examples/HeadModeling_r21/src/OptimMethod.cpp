/*! \class OptimMethod
* \brief Abstract base class used as interface for implementing optimization  methods, any optimization method must implemnt the abstract methods.

* \author Adnan Salman
* \author NeuroInformatic Center
* \date 2012
*/
#include "OptimMethod.h"

using std::vector;
using std::ostream;
using std::map;
using std::string;

OptimMethod::~OptimMethod(){}

void OptimMethod::SetMaxFcnEval( int maxFcnEval ) { 
  mMaxFcnEval = maxFcnEval; 
}

void OptimMethod::SetObjFunc(ObjFunc *pObjFunc) { 
  mpObjFunc = pObjFunc; 
};

void OptimMethod::SetVarLowerBound( vector<float> varLowerBound ) {  
  mVarLowerBound = varLowerBound;
}

void OptimMethod::SetVarUpperBound(vector<float> varUpperBound) {  
  mVarUpperBound = varUpperBound;
}

void OptimMethod::SetTolerance( float tolerance ){
  mTolerance = tolerance;
}

float OptimMethod::RandUniform01() {
  return ((float)rand())/(float)RAND_MAX;
}

float OptimMethod::RandUniform(float x1, float x2) {
  return ((float) RandUniform01()*(x2-x1)+x1);
}

int OptimMethod::GetNumberFcnEval(){ 
  return  mNumberFcnEval;
}

void OptimMethod::SetParams(map<string, vector<string> > inputParams){
  map<string, vector<string> >::iterator iter;
  string           param;
  vector<string>   pvalue;

  for (iter = inputParams.begin(); iter != inputParams.end(); iter++){
    param = iter->first;
    pvalue = iter->second;

    if      (param == "optim_tolerance")     	SetTolerance( atof(&pvalue[0][0]));
    else if (param == "max_func_eval")        SetMaxFcnEval(atoi(&pvalue[0][0]));
  }
}

