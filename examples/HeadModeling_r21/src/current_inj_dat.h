#ifndef __CUR_INJ_DAT_
#define __CUR_INJ_DAT_

#include <vector>
#include "Poisson.h"
#include <map>
#include <string>

using std::vector;
using std::string;

class CurrentInjecData{
  friend class CondInv;
  friend class CondObjFunc;

 private:
  vector<float>   mEitData;
  vector<int>     mExecludeElectrodes;  
  float           mInjectedCurrentUamp;
  float           mCalibrationConst;  
  int             mElectrodeSource;   
  int             mElectrodeSink;     

 int init_();

 public:
  CurrentInjecData();
  ~CurrentInjecData();
  CurrentInjecData(const string& rEitData);
  int LoadEitData (const string& rEitData);
  vector<float> getEitData();
  vector<int> getExcludeElectrodes();

};

#endif
