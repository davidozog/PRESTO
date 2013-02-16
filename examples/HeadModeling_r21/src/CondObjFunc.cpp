#include <iostream>
#include <cmath>
#include "Poisson.h"
#include <vector>
#include "CondObjFunc.h"
#include "HmUtil.h"
#include <fstream>

using std::endl;
using std::ofstream;

CondObjFunc::CondObjFunc(){}
CondObjFunc::~CondObjFunc(){}

float CondObjFunc::l2norm(vector<float> Pot){

  float sum = 0;

  /*
  if ( mRank == 0 ) {
    for (int i=1; i<mV.size(); i++){
      cout << i << "  " << Pot[i]*2/0.02390  << endl; 
      // cout << i << "  " << Pot[i] << "  "  << mV[i] << endl;
    }
      exit(1);
  }
  */

  for (int i=1; i<mV.size(); i++){
    sum += (Pot[i] - mV[i])*(Pot[i] - mV[i]);
  }

  float ssqrt = sqrt(sum)/(mV.size() - (1 + mExecludedElecIds.size()));

  if (isnan( ssqrt))
    HmUtil::ExitWithError("Error: NaN l2Norm result ");

  return ssqrt;
}

float CondObjFunc::relL2norm(vector<float> Pot){

  float sum = 0;
  float epsilon = 0.0001;

  for (int i=1; i<mV.size(); i++){
    if (mV[i] > epsilon)
      sum += (Pot[i] - mV[i])*(Pot[i] - mV[i]) / (mV[i]*mV[i]);
  }
  return sqrt(sum);
}

float CondObjFunc::normL2norm(vector<float> Pot){

  float sum_1=0;
  float sum_2=0;

  for (int i=1; i<mV.size(); i++){
    sum_1 += (Pot[i] - mV[i])*(Pot[i] - mV[i]);
    sum_2 += mV[i]*mV[i];
  }

  return (sum_1/sum_2)*100;
}

float CondObjFunc::rdm(vector<float> Pot){

  float sum_1=0;
  float sum_2=0;

  vector<float> p(Pot.begin(), Pot.end());
  vector<float> v(mV.begin(),   mV.end());

  for (int i=1; i<mV.size(); i++){
    sum_1 += Pot[i]*Pot[i];
    sum_2 += mV[i]*mV[i];
    //    cout << Pot[i] << "  " << mV[i] << endl;
  }
  sum_1 = sqrt(sum_1);
  sum_2 = sqrt(sum_2);
  float tot = 0;
  for (int i=1; i<mV.size(); i++){
    p[i] /= sum_1;
    v[i] /= sum_2;
    tot += (p[i] - v[i])*(p[i] - v[i]);
  }

  //  cout << "rdm = " << sqrt(tot) << endl;
  //  exit(1);

  return sqrt(tot);
}

int CondObjFunc::operator()(int N, vector<float> cond, float & fvalue) {

  //rm
  // cond[0] = .02;
  // cond[1] = .54;
  // cond[2] = .07;
  //

  mP->reinit();
  mP->SetConductivitiesAni(cond, mVarParamIndices);

  int num_iter          = mP->Solve(const_cast<string&> (mParallelism), mOmpThreadsOrDevice);    
  vector<float>     Pot = mP->SensorsPotentialVec();
  float refElectrodePot = mP->VoxelPotential(mRefElectrodePos);

  for (int i=1; i<Pot.size(); i++)
    Pot[i] = (fabs(Pot[i] - refElectrodePot));

  for (int j=0; j< mExecludedElecIds.size(); j++) Pot[mExecludedElecIds[j]] = 0.0;
  
  fvalue = (this->*mObjFuncMethod)(Pot);
  return num_iter;

}

void CondObjFunc::init(Poisson * Po,  vector<int> varParamIndices, 
		      string objFuncName, vector<int> refElectrodePos, 
		      int rank,  int ompThreadsOrDevice, string parallelism){

  mP = Po;
  mV.clear();
  mExecludedElecIds.clear();

  mRefElectrodePos = refElectrodePos;
  mVarParamIndices = varParamIndices;

  //  mObjFuncMethod = &CondObjFunc::l2norm;
  if (objFuncName == "l2norm")
    mObjFuncMethod = &CondObjFunc::l2norm;
  else if (objFuncName == "normL2norm")
    mObjFuncMethod = &CondObjFunc::normL2norm;
  else if (objFuncName == "relL2norm")
    mObjFuncMethod = &CondObjFunc::relL2norm;
  else if (objFuncName == "rdm")
    mObjFuncMethod = &CondObjFunc::rdm;
  else {
    HmUtil::ExitWithError("Unsupported objective function: " + objFuncName);
  }

  mParallelism = parallelism;
  mOmpThreadsOrDevice = ompThreadsOrDevice;
  mRank = rank;

}

void CondObjFunc::set_measured_data(const CurrentInjecData& cid){

  mP->RemoveCurrentSource("all");
  mP->AddCurrentSource( cid.mElectrodeSource, cid.mInjectedCurrentUamp, -1 );
  mP->AddCurrentSource( cid.mElectrodeSink,  -cid.mInjectedCurrentUamp, -1 );

  mV = cid.mEitData;  
  mExecludedElecIds = cid.mExecludeElectrodes;
  for ( int i=0; i<mExecludedElecIds.size(); i++ ) mV[mExecludedElecIds[i]] = 0;

}
