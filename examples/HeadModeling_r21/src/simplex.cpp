#include <iostream>
#include <iomanip>

#include "simplex.h"
#include <math.h>
#include <float.h>

using namespace std;
  
Simplex::Simplex(){}
Simplex::~Simplex(){}
Simplex::Simplex(const Simplex &sim){}

Simplex::Simplex(int numberOfVariables, ObjFunc * pObjFunc){
  Init(numberOfVariables,  pObjFunc);
}

Simplex::Simplex(int numberOfVariables, ObjFunc * pObjFunc, int rank){
  Init(numberOfVariables,  pObjFunc);
  mRank   = rank;
}

void Simplex::Init(int numberOfVariables, ObjFunc * pObjFunc){
  mNumberOfVariables = numberOfVariables;
  mpObjFunc          = pObjFunc;
  
  mMaxFcnEval        = 20000;

  mVarLowerBound.resize(mNumberOfVariables);
  mVarUpperBound.resize(mNumberOfVariables);
  mXOptimal.resize(mNumberOfVariables);
  
  for(int i = 0; i< mNumberOfVariables; i++){
    mVarLowerBound[i] = -5.0;
    mVarUpperBound[i] =  5.0;
  }
  mNumberFcnEval      = 0;
  mRank               = 0;
  mpOutStream         = NULL;
  mMaxFcnEval         = 2000;
  mTolerance          = .05;
  mSimplexEps         = 1.0e-4;
  mFOptimal           = FLT_MAX;
}

void Simplex::gensimplex(float **ps, float *p, const int ndim, const float &sz)
{
  const int mpts = ndim+1;
  for (int j = 0; j < ndim; j++)  /* set ps[0] to user's best guess, p */
    ps[0][j] = p[j]; 
  for (int i = 1; i < mpts; i++) /* build a simplex around the guess */
    {
      for (int j = 0; j < ndim; j++) 
        ps[i][j] = p[j]*(1.+sz*(sqrt(mpts*1.0)-1.)/(ndim*sqrt(2.)));
      ps[i][i-1] = ps[i][i-1]+sz*p[i-1]*(sqrt(mpts*1.0)+ndim-1)/(ndim*sqrt(2.));
    }
  return;    
}

void Simplex::get_psum(float **p, float *psum, const int ndim, const int mpts)
{
  for(int j=0;j<ndim;j++)
    {
      psum[j]=0.0;
      for (int i=0;i<mpts;i++)
        psum[j] += p[i][j];
    }
}

float Simplex::funcall(float *a){

  vector<float> point(mNumberOfVariables);
  float    F = FLT_MAX;

  bool inBound = true;
  for (int i = 0; i<mNumberOfVariables; i++){
    if (a[i] < mVarLowerBound[i] || a[i] > mVarUpperBound[i]) {
      inBound = false;
      break;
    }
  }

  if (!inBound) return F;

  for (int i = 0; i<mNumberOfVariables; i++) 
    point[i] = a[i];

  int numIterations = 0;
  if (inBound){
    numIterations = (*mpObjFunc) (mNumberOfVariables, point, F);
  }

  mNumberFcnEval++;

  if (inBound &&  mFOptimal >  F){
    mFOptimal = F;
    for (int i = 0; i<mNumberOfVariables; i++){
      mXOptimal[i] = point[i];
    }
  }
  
  
  if (mpOutStream){
    *mpOutStream  << left <<  setw(5) << mRank << setw(5) << numIterations << setw(5) << mNumberFcnEval 
		 << setw(13) << F  << setw(13) << mFOptimal;

    for (int i=0; i<mNumberOfVariables; i++) *mpOutStream << setw(13) << point[i];
    *mpOutStream << " : ";
    for (int i=0; i<mNumberOfVariables; i++) *mpOutStream << setw(13) << mXOptimal[i];
    *mpOutStream << endl;
  }

  /*

  cout  << left <<  setw(5) << mRank << setw(5) << numIterations << setw(5) << mNumberFcnEval 
  << setw(13) << F  << setw(13) << mFOptimal;
  for (int i=0; i<mNumberOfVariables; i++) cout << setw(13) << point[i];
  cout << " : ";
  for (int i=0; i<mNumberOfVariables; i++) cout << setw(13) << mXOptimal[i];
  cout << endl;
  
  */

  return (float) F;
}

float Simplex::amotry(float **p, float *y, float *psum, const int ndim, const int ihi, int &nfunk,
              const float &fac)
{
  float fac1 = (1.-fac)/ndim;
  float fac2 = fac1 - fac;
  float ptry[ndim];
  for (int j = 0; j < ndim; j++) 
    {
      ptry[j] = psum[j]*fac1-p[ihi][j]*fac2;
      
    }

  float ytry = funcall(ptry);
  if (isnan(ytry)){
    ytry = DBL_MAX;
  }
  
  ++nfunk;
  if (ytry < y[ihi])
    {
      y[ihi] = ytry;
      for (int j = 0; j < ndim; j++)
        {
          psum[j] += ptry[j] - p[ihi][j];
          p[ihi][j] = ptry[j];
        }
    }
  return(ytry);
}

void Simplex::amoeba(float *p[], float y[], const int &ndim, const float &ftol, int &nfunk, int& termination, int Nmax)
{
  const float ALPHA        = 1.0;
  const float BETA         = 0.5;
  const float GAMMA        = 2.0;
        float  psum[ndim];
  const int    mpts         = ndim+1;

  nfunk = 0;
  get_psum(p,psum,ndim,mpts);

  for (;;)
    { 
      int inhi, ilo = 1;
      int ihi = y[0]>y[1] ? (inhi=1,0) : (inhi=0,1);
      for (int i = 0; i < mpts; i++)
        {
          if (y[i] < y[ilo]) ilo = i;
          if (y[i] > y[ihi])
            {
              inhi = ihi;
              ihi = i;
            }
          else if (y[i] > y[inhi])
            if (i != ihi) inhi=i;
        }

      //      float rtol = 2.*fabs(y[ihi]-y[ilo])/(fabs(y[ihi])+fabs(y[ilo]));

      //consider this convergence condition instead of the above 
      float rtol = 2.*fabs(y[ihi]-y[ilo])/(fabs(y[ihi] > 1 ? y[ihi] : 1));

      //      *mpOutStream << rtol << "  " << ftol << endl;

      if (rtol < ftol) {
	termination = 3;
        break;
      }

      if (nfunk >= Nmax)
        {
	  termination = 1;
          cout << endl << "Termination: reached MAX_FUNC_EVALUATION.";
          return;
        }

      float ytry = amotry(p,y,psum,ndim,ihi,nfunk,-ALPHA);
      if (ytry <= y[ilo])
        ytry = amotry(p,y,psum,ndim,ihi,nfunk,GAMMA);
      else if (ytry >= y[inhi])
        {
          float ysave = y[ihi];
          ytry = amotry(p,y,psum,ndim,ihi,nfunk,BETA);
          if (ytry >= ysave)
            {
              for (int i = 0; i < mpts; i++)
                {
                  if (i != ilo)
                    {
                      for (int j = 0; j < ndim; j++)
                        {
                          psum[j] = 0.5*(p[i][j]+p[ilo][j]);
                          p[i][j]= psum[j];
                        }
		      // y[i] = (*funk)(psum);
		      y[i] = funcall(psum);
                    }
                }
              nfunk += ndim;
              get_psum(p,psum,ndim,mpts);
            }
        }
    }
}

void Simplex::PrintInfo(ostream& outs){  
  outs << left;
  outs << setw(25) << "Number of parameters: " << mNumberOfVariables << endl;
  outs << setw(25) << "Tolerance: " << mTolerance <<endl;
  outs << setw(25) << "Simplex eps: " << mSimplexEps <<endl;
  outs << setw(25) << "Max func eval: " << mMaxFcnEval <<endl;

  outs << setw(25) << "Lower bound: " << "[";
  for (int i=0; i<mVarLowerBound.size(); i++){
    outs << mVarLowerBound[i];
    if (i==mNumberOfVariables-1) outs << "]";
    else outs << "," ;
  }
  outs <<endl;
  
  outs << setw(25) << "Upper bound: " << "[";
  for (int i=0; i<mVarUpperBound.size(); i++){
    outs << mVarUpperBound[i];
    if (i==mNumberOfVariables-1) outs << "]";
    else outs << ",";
  }
  outs << endl;
}

int Simplex::Optimize(vector<float> &X, float& F_optimal, ObjFunc *of, ostream* outs){

  mpOutStream =  outs;

  int termination = 0;
  int num_eval =  simplexsearch(&X[0], X.size(), mSimplexEps, F_optimal, termination, mMaxFcnEval);

  return termination;

}


int Simplex::simplexsearch(float *a, const int ndim,
			   const float &aerr, float &fval, int& termination, int Nmax)
{

  const int mpts = ndim+1;
  float *simp[mpts],simps[mpts][ndim],y[mpts];
  int niters, niters2;
  termination = 0;

  vector<float> point;
  float F;

  for (int i = 0; i < mpts; i++)
    simp[i] = &simps[i][0];

  gensimplex(simp,a,ndim,0.1);

  for (int i = 0; i < mpts; i++){
    y[i] = funcall(simp[i]);
  }

  amoeba(simp,y,ndim,aerr,niters, termination, Nmax);
  for (int j = 0; j < ndim; j++)
    a[j] = simp[0][j];

  gensimplex(simp,a,ndim,0.1);

  for (int i = 0; i < mpts; i++){
    y[i] = funcall(simp[i]);
  }

  amoeba(simp,y,ndim,aerr,niters2, termination, Nmax);

  float funcval2 = funcall(simp[0]);
  float funcval1 = funcall(a);

  float  converr = 2.*fabs( funcval2 - funcval1);
  converr /= fabs(funcval2 + funcval1);
  if (converr > 2*aerr)
    cout << endl << "Warning: simplex converged to two different points" << endl;;

  for (int j = 0; j < ndim; j++){
    a[j] = simp[0][j];
  }
  fval = funcval2;
  return niters;    
}

void Simplex::Swap(float &rX, float& rY){
  float temp = rX;
  rX = rY;
  rY = temp;
}

void Simplex::SetParams(map<string, vector<string> > inputParams){
  OptimMethod::SetParams(inputParams);
  map<string, vector<string> >::iterator iter;
  string           param;
  vector<string>   pvalue;

  for (iter = inputParams.begin(); iter != inputParams.end(); iter++){
    param = iter->first;
    pvalue = iter->second;
    if (param == "simplex_eps")           SetSimplexEps(atof(&pvalue[0][0]));
  }
}
