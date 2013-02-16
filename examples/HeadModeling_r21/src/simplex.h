#ifndef __SIMPLEX_H__
#define __SIMPLEX_H__

#include <iostream>
#include "ObjFunc.h"
#include "OptimMethod.h"

using namespace std;

class Simplex : public OptimMethod {

 public:
  Simplex();
  ~Simplex();
  Simplex(const Simplex &sim);
  Simplex(int numberOfVariables, ObjFunc * pObjFunc);
  Simplex(int numberOfVariables, ObjFunc * pObjFunc, int rank);

  void Init(int numberOfVariables, ObjFunc * pObjFunc);

  
  virtual int  Optimize(vector<float> &rInitVariables, float& rOptimObjFunc, ObjFunc *pObjFunc, ostream* pouts);
  virtual void PrintInfo(ostream& rOuts = std::cout);
  virtual void SetParams(map<string, vector<string> > inputParams);
  void SetSimplexEps(float simplexEps){ mSimplexEps = simplexEps; }

 private:
  int            mRank;
  float          mFOptimal;
  vector<float>  mXOptimal;
  ostream        *mpOutStream;
  float          mSimplexEps;


  int simplexsearch(float *a, const int ndim,
  		    const float &aerr, float &fval, int& termination, int Nmax);
  void myswap(float &x, float& y);
  float funcall(float *a);
  void gensimplex(float **ps, float *p, const int ndim, const float &sz);
  void get_psum(float **p, float *psum, const int ndim, const int mpts);
  void printPoints(float **p, int ndim, const int mpts);
  float amotry(float **p, float *y, float *psum, const int ndim, const int ihi, int &nfunk,
	       const float &fac);
  void amoeba(float *p[], float y[], const int &ndim, const float &ftol, int &nfunk, int& termination, int Nmax);
  void Swap(float &rX, float& rY);
};

#endif
