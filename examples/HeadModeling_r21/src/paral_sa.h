#ifndef __PARA_SANNEAL___
#define __PARA_SANNEAL___


#include <iostream>
#include <cmath>
#include <stdlib.h>
#include "mpi.h"
#include "ObjFunc.h"
#include <vector>
#include <sys/time.h>
#include <iomanip>
#include <fstream>
#include "OptimMethod.h"

using namespace std;

class SimAnneal : public OptimMethod {

 private:

  float            mRt;           
  float            mSimAnnEps;     
  int              mNs;               
  int              mNt;             
  int              mCheck;    
  vector<float>    mCstep;       
  float            mUratio;
  float            mLratio;
  float            mTemp;     
  vector<float>    mVm;            

  int              mNumberAcceptedMoves;       
  int              mNumberOutOfBoundMoves; 
  int              mNumberUpMoves;
  int              mNumberDownMoves; 
  MPI_Comm         mMpiComm;
  int              mRank;

 public:
  SimAnneal();
  SimAnneal(int mNumberOfVariables, ObjFunc * pObjfunc, int rank = 0);
  SimAnneal(int mNumberOfVariables, ObjFunc * pObjfunc, MPI_Comm& condComm, int rank = 0);

  ~SimAnneal(){};

  void SetNs(int ns){ mNs = ns;}
  void SetNt(int nt){  mNt = nt; }
  void SetSimAnnEps(float simAnnEps){ mSimAnnEps = simAnnEps; }
  void SetCheck(int check){ mCheck = check;}
  void SetInitTemp(float temp){ mTemp = temp;}
  void SetRt(float rt){ mRt = rt; }
  void SetInitMaxStep(vector<float>  vm){ mVm = vm; }
  void SetInitCstep(vector<float>  cstep){mCstep = cstep;}
  void SetUratio(float uratio){mUratio = uratio;}
  void SetLratio(float lratio){mLratio = lratio;}
  void SetStepAdjParam(float uratio, float lratio, vector<float> cstep){
    mUratio = uratio; mLratio = lratio; mCstep = cstep;
  }
  
  int GetNumOutOfBound()      { return mNumberOutOfBoundMoves; }
  int GetNumberAcceptedMoves(){ return mNumberAcceptedMoves; }
  int GetNumberUpMoves()      { return mNumberUpMoves; }

  int GenerateTrialPoint(int h, int n, vector<float>  x, vector<float> & x_try, float & F_try);
  void AdjustStepSize(vector<int> nacp, int N);

  virtual int  Optimize(vector<float> &rInitVariables, float& rOptimObjFunc, ObjFunc *pObjFunc, ostream* pouts);
  virtual void SetParams(map<string, vector<string> > inputParams);
  virtual void PrintInfo(ostream& rOuts = std::cout);

 
};

#endif
