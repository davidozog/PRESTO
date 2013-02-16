#include <iostream>
#include <fstream>
#include <sstream>

#include "current_inj_dat.h"

using std::cerr;
using std::endl;
using std::ifstream;
using std::stringstream;

CurrentInjecData::CurrentInjecData(){
  init_();
}

CurrentInjecData::~CurrentInjecData(){};

CurrentInjecData::CurrentInjecData(const string& rEitData){
  init_();
  LoadEitData(rEitData);
}

int CurrentInjecData::LoadEitData(const string & rEitData){

  ifstream inm(&rEitData[0]);

  if (!inm.is_open()) {
    cerr << "IOError: Cannot open current injection data file: " << rEitData << endl;
    exit(1);
  }

  int      numberElectrodes;
  int      sources[2], id, electrodeId;
  string   line;

  inm >> numberElectrodes;
  inm >> mInjectedCurrentUamp >> mCalibrationConst;
  inm >> mElectrodeSource >>  mElectrodeSink;
  
  getline(inm, line, '\n');
  getline(inm, line, '\n');
  
  stringstream lines (std::stringstream::in | std::stringstream::out);
  lines << line;
  while (lines >> electrodeId) mExecludeElectrodes.push_back(electrodeId);

  mEitData.resize(numberElectrodes+1); 
  for (int i=0; i<numberElectrodes; i++) {
    inm >> id;
    inm >> mEitData[id];

    //note the 2 is fix the mismatch between the measured and computed data for CH current 
    //injection data set only. Remove this when using other data sets
    //** coment this line if using data sets other than the one used as example

    //calibrate and divided by two
    //  mEitData[id] = mEitData[id] * mCalibrationConst / 2.0;
 
    // no calib and p-to-p
      // mEitData[id] = mEitData[id] / 2.0;

  }
}

int CurrentInjecData::init_(){

  mEitData.clear();
  mExecludeElectrodes.clear();
  mInjectedCurrentUamp  = 1;
  mCalibrationConst     = 1;
  mElectrodeSource      = 0;
  mElectrodeSink        = 0;

};
