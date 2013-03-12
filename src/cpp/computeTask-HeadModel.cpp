/* Your includes here */
#include "HmUtil.h"
#include "Poisson.h"
#include "PoissonAdi.h"
#include "PoissonVai.h"
#include "HmTask.h"

/* Need these */
#include "env.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
using namespace std;

#define USE_BOOST true

void WriteSolution(Poisson *P, const string& outputNamePrefix, const string& solution, HmTask &task){

  string head_out_pot;
  if (solution == "head" || solution.empty()){
    head_out_pot = outputNamePrefix + "_head.dat" ;
    P->WriteSolution(&head_out_pot[0], 'b');
  }

  if (solution == "sensors" || solution.empty()){
    head_out_pot = outputNamePrefix + "_sns.txt" ;
    ofstream outs(&head_out_pot[0]);
    map<int, float> elec = P->SensorsPotentialMap();
    for (map<int, float>::const_iterator it = elec.begin(); it != elec.end(); it++){
      outs << setw(10) << it->first << "  " << setw(15) << it->second << endl;
    }
    outs.close();

  task.potentials = elec;

  }
  
}

int SolvePoissonEquationVai(HmTask &task, string taskid) {

  string datapath              = "./"; 
  string brainkDataDir         = "";   
  string brainkDataKind        = "";   
  string dataSetPrefix         = "";
  string outputNamePrefix      = "";  

  string geometryFileName      = "";   
  string sensorsFileName       = "";
  string normalsFileName       = "";
  string algorithm             = "";

  float  tolerance             = 0.001;
  int    maxNumIterations      = 2000;
  float  timeStep              = 2.8; 

  float  convEps               = 0.0001;
  int    convCheck             = 10;

  float  tang_to_normal_ration = 1.0;
  float  current               = 1.0;

  vector<int> curr_src;
  vector<int> curr_snk;

  HeadModel headModel;
  string    parallelism = "omp";
  string    solution    = "";

  string         param;
  vector<string> pvalue;
  float skull_normal_cond = -1;
  bool boneDensityMode = false;

  map<string, vector<string> >::iterator iter;

  for (iter = task.params.begin(); iter != task.params.end(); iter++){

    param  = iter->first;
    pvalue = iter->second;
    HmUtil::TrimStrSpaces(param);

    if (param == "datapath") {
      datapath = pvalue[0];
      if (datapath[datapath.size()-1] != '/') datapath += "/";
      HmUtil::TrimStrSpaces(datapath);

      if (datapath[0] != '/'){
	datapath = string(getenv ("HEAD_MODELING_HOME"))+"/"+ datapath;
      }
    }

    else if (param == "braink_data_dir")       brainkDataDir         = pvalue[0];
    else if (param == "data_set_kind")         brainkDataKind        = pvalue[0]; 
    else if (param == "algorithm")             algorithm             = pvalue[0]; 
    else if (param == "parallelism")           parallelism           = pvalue[0]; 
    else if (param == "geometry")              geometryFileName      = pvalue[0];
    else if (param == "output_name_prefix")    outputNamePrefix      = pvalue[0];
    else if (param == "sensors")               sensorsFileName       = pvalue[0];
    else if (param == "normals")               normalsFileName       = pvalue[0];
    else if (param == "tol")                   tolerance             = atof(&pvalue[0][0]);
    else if (param == "skull_normal_cond")     skull_normal_cond     = atof(&pvalue[0][0]);
    else if (param == "tang_to_normal_ration") tang_to_normal_ration = atof(&pvalue[0][0]);
    else if (param == "convergence_check")     convCheck             = atoi(&pvalue[0][0]);
    else if (param == "convergence_eps")       convEps               = atof(&pvalue[0][0]);
    else if (param == "time_step")             timeStep              = atof(&pvalue[0][0]);
    else if (param == "max_iter")              maxNumIterations      = atof(&pvalue[0][0]);
    else if (param == "solution")              solution              = pvalue[0];
    else if (param == "bone_density_mode")     boneDensityMode       = bool(atoi(&pvalue[0][0]));
    else if (param == "current")               current               = atof(&pvalue[0][0]); 
    else if (param == "current_src") {
      for (unsigned int i=0; i<pvalue.size(); i++){
	curr_src.push_back(atoi(&pvalue[i][0]));
      }
    }

    else if (param == "current_sink") {
      for (unsigned int i=0; i<pvalue.size(); i++){
	curr_snk.push_back(atoi(&pvalue[i][0]));
      }
    }
  }

  //  Create head model 
  if (brainkDataDir.empty()) {

    if (geometryFileName.empty()) HmUtil::ExitWithError( "IOError: no geometry data file specified ... " );
    if (sensorsFileName.empty())  HmUtil::ExitWithError( "IOError: no sensors data file specified ... ");

    geometryFileName = datapath + geometryFileName;
    sensorsFileName = datapath + sensorsFileName;

    if (!normalsFileName.empty()){
      normalsFileName = datapath + normalsFileName;
    }

    int err = headModel.init_fdm(geometryFileName, sensorsFileName, "", "", normalsFileName, dataSetPrefix);
    if (err) HmUtil::ExitWithError("IOError: loading brainK data ... data");

  }

  else{
    string brainkDataPath = datapath + brainkDataDir; 
    int err = headModel.Init(brainkDataPath, brainkDataKind, dataSetPrefix);
    if (err) HmUtil::ExitWithError("IOError: loading brainK data ... " + brainkDataPath);
  }


  //  create Poisson solver object

  Poisson *P = NULL;
  HmUtil::TrimStrSpaces(algorithm);

  bool isVAI = false;

  if (algorithm == "vai") {
    P = new PoissonVAI();
    isVAI = true;
  }
  else if (algorithm == "adi")
    P = new PoissonADI();
  else {
    HmUtil::ExitWithError("IOError: Poisson, unsupported forward algorithm ... " + algorithm);
  }

  if(P->SetHeadModel(headModel))
    HmUtil::ExitWithError("IOError: Poisson, error setting geometry, sensors or dipoles ... ");

  P->init();
  P->SetTangentToNormalRatio(tang_to_normal_ration);
  P->SetSkullNormalCond(skull_normal_cond);

  //  P->init();
  P->SetConvTolerance(tolerance);
  P->SetTimeStep(timeStep);
  P->SetMaxIterations(maxNumIterations);
  P->SetConvEps(convEps);
  P->SetConvCheck(convCheck);

  cout << "boneDensityMode: " << boneDensityMode << endl;

  P->SetBoneDensityMode(boneDensityMode);

  vector<string> init_tissue_names;   
  vector<string> init_tissue_conds;
  
  if (task.params.find("tissues") != task.params.end())
    init_tissue_names = task.params["tissues"];
  
  if (task.params.find("tissues_conds") != task.params.end())
    init_tissue_conds = task.params["tissues_conds"];

  for (int i=0; i<init_tissue_names.size(); i++){
    if (P->SetTissueConds(init_tissue_names[i], atof(&init_tissue_conds[i][0])))
      HmUtil::ExitWithError("IOError: unrecognized tissue ... " + init_tissue_names[i]);
  }

  P->RemoveCurrentSource("all");
  bool sns_avail = headModel.AvailableSensors();

  if (curr_src.empty() || curr_snk.empty())
    HmUtil::ExitWithError("No current source-sink specified");

  cout << current << endl;
  cout << "Current source: "; 
  for (unsigned int i=0; i<curr_src.size(); i++){
    cout << curr_src[i] << "  ";
  }
  cout << endl;

  cout << "Current sink: ";
  for (unsigned int i=0; i<curr_snk.size(); i++){
    cout << curr_snk[i] << "  ";
  }
  cout << endl;

  if (curr_src.size() < 1 || curr_src.size() > 3){
    HmUtil::ExitWithError("IOError: incorrect current source location ... ");
  }
  else if (curr_src.size() == 1){
    if (!sns_avail){
      HmUtil::ExitWithError("IOError: incorrect current source location, no sensors specified ... ");
    }
    //    P->AddCurrentSource(curr_src[0], current);
    P->AddCurrentSource(curr_src[0], current);
  }
  else{
    //    P->AddCurrentSource(&curr_src[0], current);
    cout <<  "current source in tissue: " << P->GetTissueConds(curr_src) << endl;
    P->AddCurrentSource(&curr_src[0], current);
  }

  if (curr_snk.size() < 1 || curr_snk.size() > 3){
    HmUtil::ExitWithError("IOError: incorrect current sink location ... ");
  }

  else if (curr_snk.size() == 1){
    if (!sns_avail){
      HmUtil::ExitWithError("IOError: incorrect current sink location, no sensors specified ... ");
    }
    P->AddCurrentSource(curr_snk[0], -current);
  }
  else{
    cout <<  "current sink in tissue: " << P->GetTissueConds(curr_snk) << endl;
    P->AddCurrentSource(&curr_snk[0], -current);

  }

  int cudaGPUCount = HmUtil::GetCudaDevicesCount();// P->GetCudaDevicesCount();
  int device_or_threads = -1;

  if (parallelism == "cuda"){
    device_or_threads = 0;
    if (cudaGPUCount <= 0){ //cuda is not supported 
      HmUtil::ExitWithError("IOError: No cuda devices supported  ... ");
    }
  }
  else if (parallelism != "omp")  //either omp or cuda
    HmUtil::ExitWithError("IOError: Unsupported parallelism  ... ");

  /////////////////////////////////////////////////////////////////////////
  // Solve poisson equation and write output files                     ////
  // 0 threads means all threads, 0 device means use gpu device 0      ////
  /////////////////////////////////////////////////////////////////////////

  if (solution == "sensors" && !sns_avail){
    HmUtil::ExitWithError("IOError: no sensors specified, no output at sensors ... ");
  }

  P->PrintInfo(cout);
  int numIter = P->Solve(parallelism, device_or_threads);
  WriteSolution(P, taskid.c_str(), solution, task);

  /*
  int cont = 1;
  while (cont){
    cout << "Continue: ";
    cout.flush();
    cin >> cont;
    if (!cont) break;

    cout << "Tangent to Normal ratio: ";
    cout.flush();
    cin >> tang_to_normal_ration;

    cout << "Skull radial: ";
    cout.flush();
    cin >> skull_normal_cond;

    P->reinit();
    P->SetTangentToNormalRatio(tang_to_normal_ration);
    P->SetSkullNormalCond(skull_normal_cond);
    P->printInfo(cout);
    numIter = P->Solve(parallelism, device_or_threads);
    WriteSolution(P, outputNamePrefix, solution);
  }

  */

  delete P;
  P = NULL;

  return 0;
}


void restoreObject(HmTask &task, const char * filename) {
    std::ifstream ifs(filename);
    boost::archive::text_iarchive ia(ifs);
    ia >> task;
}


void saveObject(HmTask &task, const char * filename) {
    // make an archive
    ofstream ofs(filename);
    boost::archive::text_oarchive oa(ofs);
    oa << task;
}


string computeTask(string function_name, string serialized_file, string taskid) {

  cout << "function name: " << function_name << endl;

  cout << "datafile: " << serialized_file << endl;


  HmTask task;

  if (USE_BOOST) {
    restoreObject(task, serialized_file.c_str());
  }
  else {
    ifstream ifs(serialized_file.c_str(), ios::binary);
    ifs.read((char *)&task, sizeof(task));
  }

  /* this is a constructor call: */
  map<string, vector<string> >  params (task.params);
  string         param;
  vector<string> pvalue;

  map<string, vector<string> >::iterator iter;
  for (iter = params.begin(); iter != params.end(); iter++){
    param  = iter->first;
    pvalue = iter->second;
    cout << param << " : " << pvalue[0] << endl;
  }


  /* Compute forward solution: */
  SolvePoissonEquationVai(task, taskid);


  /* TODO: this should be a Presto function - "saveResults" or something */
  Env env;
  string uid = env.getUid();
  cout << "uid is : " << uid << endl;

  string results_filename = "/dev/shm/." + uid + "_r" + taskid + ".mat";

  /* TODO: Note, this is assuming T and O are the same class in Master, generally won't be the case */
  if (USE_BOOST) {
    saveObject(task, results_filename.c_str());
  }
  else {
    ofstream ofs(results_filename.c_str(), ios::binary);
    ofs.write((char *)&task, sizeof(task));
    ofs.close();
  }
  /* end function */

  return results_filename;

}
