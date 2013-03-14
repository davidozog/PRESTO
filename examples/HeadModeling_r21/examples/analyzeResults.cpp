#include <iostream>
#include <fstream>
#include "HmTask.h"
#include "current_inj_dat.h"
#include <boost/archive/text_iarchive.hpp>

using namespace std;

void restoreObject(HmTask &task, const char * filename) {
    std::ifstream ifs(filename);
    boost::archive::text_iarchive ia(ifs);
    ia >> task;
}

float normL2norm(map<int, float> sim, vector<float> ciV){

  float sum_1=0;
  float sum_2=0;
  int chan;
  float pot;

  map<int,float>::iterator iter;
  for (iter = sim.begin(); iter != sim.end(); iter++){
    chan = iter->first;
    pot = iter->second;

    sum_1 += (pot - ciV[chan])*(pot - ciV[chan]);
    sum_2 += ciV[chan]*ciV[chan];
  }

  return (sum_1/sum_2)*100;
}

int main(int argc, char** argv) {

  char *result_file = argv[1];

  HmTask task;

  restoreObject(task, result_file);
  
  /* this is a constructor call: */
  map<int,float>  params (task.potentials);
  int chan;
  float pot;

  map<int,float>::iterator iter;
  for (iter = params.begin(); iter != params.end(); iter++){
    chan = iter->first;
    pot = iter->second;
    //cout << chan << " : " << pot << endl;
  }

  CurrentInjecData ci;

  ci.LoadEitData("/home11/ozog/Data/s108/CI/108_0691_128Ch_Redux/108_0691_i103s45_22microA_24Hz_128CH_ELEFIX.ci");
  
  vector<float> ci_pots = ci.getEitData();
  vector<int> ci_excl = ci.getExcludeElectrodes();

  vector<float>::iterator it;
  for (it= ci_pots.begin(); it!= ci_pots.end(); it++){
    //cout << "ci_pot : " << *it << endl;
  }
  //cout << endl;

  /* Set all bad electrodes in simulation to zero */
  //cout << "exclude: " << endl;
  vector<int>::iterator itt;
  for (itt = ci_excl.begin(); itt!= ci_excl.end(); itt++){
    //cout << *itt << " ";
    try {
      params[*itt] = 0.0;
    }
    catch (int e) {
      //cout << "exception: " << e << endl;
    }
  }
  //cout << endl;

  
  /*
  for (iter = params.begin(); iter != params.end(); iter++){
    chan = iter->first;
    pot = iter->second;
    cout << chan << " : " << pot << endl;
  }
  */


  cout << normL2norm(params, ci_pots) << endl;


}
