#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

#include "PoissonAdi.h"
#include "paral_sa.h"
#include "CondObjFunc.h"
#include "cond_class.h"
#include "current_inj_dat.h"
#include "PoissonVai.h"
#include "HmUtil.h"

#include "mpi.h"
#include <iterator>
#include <omp.h>

CondInv::CondInv() {
  Init_();
}

CondInv::CondInv(int rank){
  Init_();
  mMpiComm = MPI_COMM_WORLD;
  mRank = rank;
}

CondInv::CondInv(map<string, vector<string> > inputParams, int rank) {
  Init_();
  mRank = rank;

  mMyTask = (ProcessTask) Init(inputParams);
}

CondInv::~CondInv(){
  if (mpPoissonSolver)  {delete mpPoissonSolver; mpPoissonSolver=NULL;}
  if (mpOptimizer)      {delete mpOptimizer; mpOptimizer=NULL;}
  if (mpObjFunc)        {delete mpObjFunc; mpObjFunc=NULL;}
}

int CondInv::Init_(){

  mInitConds.clear();
  mTissueLowerBound.clear();
  mTissueUpperBound.clear();
  mVarTissuesIdx.clear();
  mVarTissuesInitConds.clear();

  mRefElectrodePos.clear();
  mInputParams.clear();
  mRank = 0;

#ifdef CUDA_ENABLED
  mCompMode = "cuda";
#else
  mCompMode = "omp";
#endif
  
  mDataPath       = "./"; 
  mGeomFileName   = ""; 
  mSnsFileName    = "";
  mBrainkDataDir  = "";
  mBrainkDataKind = "";

  mpOptimizer       = NULL;

  mpPoissonSolver = NULL;
  mpObjFunc       = NULL;

  mRandSeed       = (unsigned)time( 0 );

  mObjFuncMeth    = "l2norm"; 
  mOptimMethName  = "simplex";

  mAnisotropicModel = false;

  return 0;
}


// Create a communicator to be used for conductivity optimization 
// from a subset of world communicator. We are not using all world processes
// since on some platforms (e.g. aciss) we need to request all node processors 
// to reserve the node from other users and we can't use all of them in the 
// computation for performance reasons.
//
// The approach in determining filtering the processes to be used is as follows 
// 1) Each process determine whether it can do work or not
//    a) in case of the GPU computation only number of GPUs on a node are used
//    b) in case of OMP the number of processes used is determined such that each 
//       process will have about 6-8 cores for forward calculation 
//
// 2) Each inform the master (rank 0) of its capability of doing work
//
// 3) master filters the processes that can do work such that 
//    a) in case of simulated annealing the size of intermediate loop (ns) should 
//       be a multiple of the number of participating processes to keep the amount 
//       of work balanced 
//    b) in case of simplex all capable processes can participate 
                       
void CondInv::Parallelism (string& parallelism) {
  
  int numCudaDevices  = HmUtil::GetCudaDevicesCount();

  if (numCudaDevices <= 0 && parallelism == "cuda"){
    HmUtil::ExitWithError("No cuda devices available ");
  }

  else if ((parallelism == "auto" && numCudaDevices > 0) ||
	   (parallelism == "cuda" && numCudaDevices > 0) ){

    parallelism = "cuda";

  }

  else if ((parallelism == "auto" && numCudaDevices < 0) || 
	   parallelism == "omp" ){
    parallelism = "omp";
  }

  else {
    HmUtil::ExitWithError("Usupported parallelism ");
  }
}

void CondInv::GetMyWorkKind (map<string, vector<int> >& nodes, int rank,
			     int& deviceIdOrNumThreads, const string& parallelism, 
			     int nprocs, const string& host_name, ProcessTask& myTask ) {

  deviceIdOrNumThreads  = 0;
  int numCudaDevices  = HmUtil::GetCudaDevicesCount();
  int myorder = find(nodes[host_name].begin(), nodes[host_name].end(), rank) - 
    nodes[host_name].begin();

  if (parallelism == "cuda") {
    if (myorder < numCudaDevices && myorder < nodes[host_name].size()){
      myTask = WORKER;
      deviceIdOrNumThreads = myorder;
    }
    else {
      myTask = NOTHING;
      deviceIdOrNumThreads = -1;
    }
  }

  else { //omp
    int numRanks = nodes[host_name].size();
    int numCores = omp_get_num_procs();
    int nr = numRanks;
    int coresPerRank = numCores / nr;

    while (coresPerRank < 6){
      nr--;
      coresPerRank = numCores / nr;
    }

    int left = numCores%nr;
    if (myorder < left) coresPerRank++;

    if (myorder < nr) {
      deviceIdOrNumThreads =  coresPerRank;
      myTask = WORKER;
    }
    else{
      myTask = NOTHING;
      deviceIdOrNumThreads =  -1;
    }
  }

}

void CondInv::PrintRanksInNodes (const map<string, vector<int> >& nodes, const string& prefix) const {

  map<string, vector<int> >::const_iterator it = nodes.begin();
  for (; it != nodes.end(); it++){
    cout << prefix + "Node: " << it->first << " : " + prefix + " Ranks : " 
	 << it->second.size() << " [ ";
    for (int i=0; i<it->second.size(); i++){
      cout << it->second[i] << "  ";
    }
    cout << "]" << endl;
  }

}

void CondInv::FilterWorkers (const string& optimMethodName, 
			     vector<ProcessTask>& ranksProposedWork, 
			     vector<int>& commRanks, 
			     map<string, vector<int> >& commNodesRanks, 
			     vector<string>& hostNames, int ns) {

  for (int i=0; i<ranksProposedWork.size(); i++){
      if (ranksProposedWork[i] == WORKER){
	commRanks.push_back(i);
	commNodesRanks[hostNames[i]].push_back(i);
      }
  }

  if (optimMethodName == "sa") {

    int optimN;
    int nt = commRanks.size();
    optimN = nt;
    while ( ns % optimN != 0) optimN--;

    for (int jj = 0; jj<nt-optimN; jj++){
      
      map<string, vector<int> >::iterator iter    = commNodesRanks.begin();
      map<string, vector<int> >::iterator maxNode = commNodesRanks.begin();
      
      for (; iter != commNodesRanks.end(); iter++) {
	if (iter->second.size() > maxNode->second.size()){
	  maxNode = iter;
	}
      }

      int x = maxNode->second.back();
      maxNode->second.pop_back();
      commRanks.erase(find(commRanks.begin(), commRanks.end(), x));
    }

    fill(ranksProposedWork.begin(), ranksProposedWork.end(), NOTHING);

    for (int i=0; i<commRanks.size(); i++){
      ranksProposedWork[commRanks[i]] = WORKER;
    }
  }

}

bool CondInv::MakeComunicator (string& parallelism, int& deviceIdOrNumThreads, 
				      MPI_Comm& cond_comm1, int ns, 
				      const string& optimMethodName  ) {
  int nprocs, rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  vector<string>      hostNames   = HmUtil::GetNodeOfRank();
  map<string, vector<int> > nodes = HmUtil::GetRanksOfNode(hostNames);
  if (rank == 0) PrintRanksInNodes(nodes);

  Parallelism (parallelism);
  ProcessTask myTask = NOTHING;

  GetMyWorkKind (nodes,	rank, deviceIdOrNumThreads, parallelism, nprocs, hostNames[rank], myTask );

  vector<int>         ranksDeviceOrThreads(nprocs, -1);
  vector<ProcessTask> ranksProposedWork(nprocs, NOTHING);

  MPI_Gather (&myTask, 1, MPI_INT, &ranksProposedWork[0], 1, MPI_INT, 0, 
	      MPI_COMM_WORLD);

  MPI_Gather (&deviceIdOrNumThreads, 1, MPI_INT, &ranksDeviceOrThreads[0], 1, 
	      MPI_INT, 0, MPI_COMM_WORLD);
 
  vector<int> commRanks;
  map<string, vector<int> > commNodesRanks;
  if ( rank == 0 ) FilterWorkers ( optimMethodName, ranksProposedWork, 
				   commRanks, commNodesRanks,  hostNames, ns );
 
  MPI_Bcast (&ranksProposedWork[0], (int) ranksProposedWork.size(), 
	     MPI_INT, 0, MPI_COMM_WORLD) ;

  if (rank == 0)  PrintRanksInNodes(commNodesRanks, "Active ");

  myTask = ranksProposedWork[rank];
  if (myTask == NOTHING) deviceIdOrNumThreads = -1;

  vector<int> cond_ranks;
  for (int i=0; i<ranksProposedWork.size(); i++){
    if (ranksProposedWork[i] == WORKER){
      cond_ranks.push_back(i);
    }
  }

  HmUtil::CreateMpiComm(cond_ranks, cond_comm1);
  return (myTask == WORKER);

}

int CondInv::Init(map<string, vector<string> > inputParams){

  mInputParams = inputParams;

  int        deviceIdOrMpiThreads;
  string     parallelism = "auto";
  MPI_Comm   optimCommunicator;

  if (mInputParams.find("optim_method") !=  mInputParams.end()){
    mOptimMethName = mInputParams["optim_method"][0];
  }

  if (mInputParams.find("parallelism") !=  mInputParams.end()){
    parallelism = mInputParams["parallelism"][0];
  }

  int ns = 12;
  if (mInputParams.find("ns") !=  mInputParams.end()){
    ns = atoi(&mInputParams["ns"][0][0]);
  }
  

  bool joined =  MakeComunicator (parallelism, deviceIdOrMpiThreads, optimCommunicator, 
				  ns, mOptimMethName );

  // process that are not in the communicator should return and do nothi
  if ( !joined ) return NOTHING;

  mMpiComm = optimCommunicator;
  
  MPI_Comm_rank( mMpiComm, &mRank);

  CreateForwardSolver();                //create forward solver

  //set random generator seed. Used only for sim annealing
  if (mInputParams.find("rand_seed") != mInputParams.end()){
    mRandSeed = (unsigned int) atoi(&mInputParams["rand_seed"][0][0]);
  }

  //find and the variable parameters to optimize
  SetVariableTissues();
  
  //set the objective function 
  if (mInputParams.find("objective_func") !=  mInputParams.end())
    mObjFuncMeth = mInputParams["objective_func"][0];

  mpObjFunc = new CondObjFunc();

  if (mInputParams.find("optim_method") !=  mInputParams.end()){
    mOptimMethName = mInputParams["optim_method"][0];
  }

  if (mOptimMethName != "simplex" && mOptimMethName != "sa"){
    string err = "IOError: unrecognized optim method ... " + mOptimMethName;
    HmUtil::ExitWithError( err );
  }

  mpObjFunc->init(mpPoissonSolver, mVarTissuesIdx, mObjFuncMeth, 
		  mRefElectrodePos, mRank, deviceIdOrMpiThreads,  
		  parallelism);

  srand( mRandSeed ); 
  return WORKER;

}

int CondInv::CreateForwardSolver(){
  //TODO: Move forward method setting parameters to Poisson class instead 

  string          param;
  vector<string>  paramValue;
  float           tol                   = 0.001;
  float           time_step             = 3.0; 
  int             max_iter              = 2000;

  string          algorithm             = "";
  float           convEps               = .0001;
  int             convCheck             =  10;
  float           tang_to_normal_ration =  1;
  float           skull_normal_cond     = -1;
  bool            boneDensityMode       = false;

  map<string, vector<string> >::iterator iter;
  for (iter = mInputParams.begin(); iter != mInputParams.end(); iter++){
    param = iter->first;
    paramValue = iter->second;

    if (param == "datapath") {
      mDataPath = paramValue[0];
      HmUtil::TrimStrSpaces(mDataPath);
      if (mDataPath[mDataPath.size()-1] != '/') mDataPath += "/";
      if (mDataPath[0] != '/'){
	mDataPath = string(getenv ("HEAD_MODELING_HOME"))+"/"+ mDataPath;
      }
    }

    else if (param == "algorithm")        algorithm       = paramValue[0];
    else if (param == "sensors")          mSnsFileName    = paramValue[0]; //
    else if (param == "skull_normal_cond") 
      skull_normal_cond = atof(&paramValue[0][0]);
    else if (param == "tang_to_normal_ration") 
      tang_to_normal_ration = atof(&paramValue[0][0]);
    else if (param == "convergence_check")  convCheck = atoi(&paramValue[0][0]);
    else if (param == "convergence_eps")    convEps = atof(&paramValue[0][0]);
    else if (param == "tol")                tol = atof(&paramValue[0][0]);
    else if (param == "time_step")          time_step = atof(&paramValue[0][0]);
    else if (param == "max_iter")           max_iter = atof(&paramValue[0][0]);
    else if (param == "bone_density_mode")  boneDensityMode  = bool(atoi(&paramValue[0][0]));
  }

  HeadModel   bk;
  bk.Init(mInputParams);

  vector<vector<float> > normals = bk.GetNormals();
  if (!normals.empty() && algorithm != "adi"){
    mAnisotropicModel = true;
  }

  map<string, vector<int> > refSensorsMap = bk.GetReferenceSensorsMap();
  map<string, vector<int> >::iterator loc = refSensorsMap.find("Cz");  //find 
  
  if (loc == refSensorsMap.end()){
    HmUtil::ExitWithError("IOError: unspecified reference electrode ... ");
  }

  mRefElectrodePos = loc->second;
  HmUtil::TrimStrSpaces(algorithm);

  if (algorithm == "vai") {
    mpPoissonSolver = new PoissonVAI();
  }
  else if (algorithm == "adi")
    mpPoissonSolver = new PoissonADI();
  else {
    HmUtil::ExitWithError("IOError: unsupported forward algorithm: "+algorithm);
  }

  if(mpPoissonSolver->SetHeadModel(bk))
    HmUtil::ExitWithError("IOError: error setting geometry, sensors or dipoles. ");

  mpPoissonSolver->init();
  mpPoissonSolver->SetConvTolerance(tol);
  mpPoissonSolver->SetTimeStep(time_step);
  mpPoissonSolver->SetMaxIterations(max_iter);

  mpPoissonSolver->SetConvEps(convEps);
  mpPoissonSolver->SetConvCheck(convCheck);
  mpPoissonSolver->SetBoneDensityMode(boneDensityMode);

  vector<string> init_tissue_names;   
  vector<string> init_tissue_conds;

  if (mInputParams.find("tissues") != mInputParams.end())
    init_tissue_names = mInputParams["tissues"];

  if (mInputParams.find("tissues_conds") != mInputParams.end())
    init_tissue_conds = mInputParams["tissues_conds"];

  for (int i=0; i<init_tissue_names.size(); i++){
    if (mpPoissonSolver->SetTissueConds(init_tissue_names[i], 
					atof(&init_tissue_conds[i][0])))
      HmUtil::ExitWithError("IOError: unrecognized tissue ... " + 
			    init_tissue_names[i]);
  }

  mInitConds = mpPoissonSolver->GetTissueConds();;

  if (skull_normal_cond < 0){
    skull_normal_cond = mpPoissonSolver->GetTissueConds("skull");
  }

  mInitConds.push_back(skull_normal_cond);
  mInitConds.push_back(tang_to_normal_ration);

  mPoissonSolverAlgorithm = algorithm;

  return 0;
}

/*
int CondInv::SetVariableTissues(){

  vector<string> tissueNames = mpPoissonSolver->GetTissueNames(); 
  mNumIsoTissues = tissueNames.size();

  if (mInputParams.find("variable_tissues") !=  mInputParams.end()){
    for (int i =0; i<tissueNames.size(); i++){
      if (find(mInputParams["variable_tissues"].begin(), 
	       mInputParams["variable_tissues"].end(), tissueNames[i]) 
	  != mInputParams["variable_tissues"].end()){
	mVarTissuesIdx.push_back(i);
      }
    }

    if (mVarTissuesIdx.empty())
      HmUtil::ExitWithError("IOError: no parameters to optimize ... " );
  }

  else{
    for (int i =0; i<tissueNames.size(); i++){
      mVarTissuesIdx.push_back(i);
    }
  }

  //set default lb and ub as .001 (skull) and 2.2 (CSF)
  mTissueLowerBound.resize(mVarTissuesIdx.size(), .001);
  mTissueUpperBound.resize(mVarTissuesIdx.size(), 2.2);

  //////// anisotropic parameter if the model is anisotropic 

  if (mAnisotropicModel){
    if (mInputParams.find("variable_tissues") !=  mInputParams.end()){
      if (find(mInputParams["variable_tissues"].begin(), 
	       mInputParams["variable_tissues"].end(), "skull_normal_cond")
	  != mInputParams["variable_tissues"].end()){
	mVarTissuesIdx.push_back(mNumIsoTissues);
	mTissueLowerBound.push_back(.001);
	mTissueUpperBound.push_back(.2);
      }

      if (find(mInputParams["variable_tissues"].begin(), 
	       mInputParams["variable_tissues"].end(), "tang_to_normal_ration") 
	  != mInputParams["variable_tissues"].end()){
	mVarTissuesIdx.push_back(mNumIsoTissues+1);
	mTissueLowerBound.push_back(1);
	mTissueUpperBound.push_back(6);
      }
    }
  }

  /////////////

  if (mInputParams.find("lower_bound") !=  mInputParams.end()){
    for (int i =0; i<mVarTissuesIdx.size(); i++)
      mTissueLowerBound[i] = (atof(&mInputParams["lower_bound"][i][0]));
  }
  
  if (mInputParams.find("upper_bound") !=  mInputParams.end()){
    for (int i =0; i<mVarTissuesIdx.size(); i++)
      mTissueUpperBound[i] = (atof(&mInputParams["upper_bound"][i][0]));
  }

  //set the variables initial conductivities
  mVarTissuesInitConds.resize(mVarTissuesIdx.size());
  for (int i=0; i<mVarTissuesIdx.size(); i++) 
    mVarTissuesInitConds[i] = mInitConds[mVarTissuesIdx[i]];

  //force the initial conductivities to be within the allowed of range
  for (int i=0; i<mVarTissuesInitConds.size(); i++)
    if (mVarTissuesInitConds[i] > mTissueUpperBound[i] || 
	mVarTissuesInitConds[i] < mTissueLowerBound[i]) 
      mVarTissuesInitConds[i] = mTissueLowerBound[i] + 
	(mTissueUpperBound[i] - mTissueLowerBound[i])*HmUtil::Uniform01();

  // for simplex algorithm generate a new initial 
  // values for all process except for the master

  if (mOptimMethName == "simplex" && mRank != 0){
    srand(mRandSeed + 10*mRank);
    for (int i=0; i<mVarTissuesInitConds.size(); i++){
      mVarTissuesInitConds[i] = mTissueLowerBound[i] + 
	(mTissueUpperBound[i] - mTissueLowerBound[i])*HmUtil::Uniform01();
      mInitConds[mVarTissuesIdx[i]] =  mVarTissuesInitConds[i];
    }
  }
  return 0;
}
*/

int CondInv::SetVariableTissues(){

  vector<string> tissueNames = mpPoissonSolver->GetTissueNames(); 
  mNumIsoTissues = tissueNames.size();

  //  ostream_iterator<string> out_str (cout," ");
  //  ostream_iterator<int> out_int (cout," ");
  //  ostream_iterator<float> out_fl (cout," ");
  //  copy(tissueNames.begin(), tissueNames.end(), out_str); cout<< endl;

  bool varTissueExist = false;

  if (mInputParams.find("variable_tissues") !=  mInputParams.end()){

    vector<string> varTissuesNames =  mInputParams["variable_tissues"];
    //    copy(varTissuesNames.begin(), varTissuesNames.end(), out_str); cout<< endl;

    mVarTissuesIdx.resize(varTissuesNames.size(), -1);

    for (int j=0; j<varTissuesNames.size(); j++){
      string varTissue =  varTissuesNames[j];
      HmUtil::TrimStrSpaces(varTissue);
      varTissueExist = false;

      for (int i=0; i<tissueNames.size(); i++){
	if (varTissue == tissueNames[i]) {
	  mVarTissuesIdx[j] = i;
	  varTissueExist = true;
	  break;
	}
      }

      if (!varTissueExist){
	HmUtil::ExitWithError("Unrecognized variable tissue " + varTissue);
      }
      
    }

    //    copy(mVarTissuesIdx.begin(), mVarTissuesIdx.end(), out_int); cout<< endl;

    if (mVarTissuesIdx.empty())
      HmUtil::ExitWithError("IOError: no parameters to optimize ... " );

    if (find(mVarTissuesIdx.begin(), mVarTissuesIdx.end(), -1) != mVarTissuesIdx.end()){
      HmUtil::ExitWithError("Duplicate var tissues specified " );
    }
  }

  else{
    for (int i =0; i<tissueNames.size(); i++){
      mVarTissuesIdx.push_back(i);
    }
  }

  //set default lb and ub as .001 (skull) and 2.2 (CSF)
  mTissueLowerBound.resize(mVarTissuesIdx.size(), .001);
  mTissueUpperBound.resize(mVarTissuesIdx.size(), 2.2);

  //////// anisotropic parameter if the model is anisotropic 

  if (mAnisotropicModel){
    if (mInputParams.find("variable_tissues") !=  mInputParams.end()){
      if (find(mInputParams["variable_tissues"].begin(), 
	       mInputParams["variable_tissues"].end(), "skull_normal_cond")
	  != mInputParams["variable_tissues"].end()){
	mVarTissuesIdx.push_back(mNumIsoTissues);
	mTissueLowerBound.push_back(.001);
	mTissueUpperBound.push_back(.2);
      }

      if (find(mInputParams["variable_tissues"].begin(), 
	       mInputParams["variable_tissues"].end(), "tang_to_normal_ration") 
	  != mInputParams["variable_tissues"].end()){
	mVarTissuesIdx.push_back(mNumIsoTissues+1);
	mTissueLowerBound.push_back(1);
	mTissueUpperBound.push_back(6);
      }
    }
  }

  /////////////

  if (mInputParams.find("lower_bound") !=  mInputParams.end()){
    for (int i =0; i<mVarTissuesIdx.size(); i++)
      mTissueLowerBound[i] = (atof(&mInputParams["lower_bound"][i][0]));
  }
  
  if (mInputParams.find("upper_bound") !=  mInputParams.end()){
    for (int i =0; i<mVarTissuesIdx.size(); i++)
      mTissueUpperBound[i] = (atof(&mInputParams["upper_bound"][i][0]));
  }

  //  copy(mTissueLowerBound.begin(), mTissueLowerBound.end(), out_fl); cout<< endl;
  //  copy(mTissueUpperBound.begin(), mTissueUpperBound.end(), out_fl); cout<< endl;

  //set the variables initial conductivities
  mVarTissuesInitConds.resize(mVarTissuesIdx.size());
  for (int i=0; i<mVarTissuesIdx.size(); i++) 
    mVarTissuesInitConds[i] = mInitConds[mVarTissuesIdx[i]];

  //force the initial conductivities to be within the allowed of range
  for (int i=0; i<mVarTissuesInitConds.size(); i++)
    if (mVarTissuesInitConds[i] > mTissueUpperBound[i] || 
	mVarTissuesInitConds[i] < mTissueLowerBound[i]) 
      mVarTissuesInitConds[i] = mTissueLowerBound[i] + 
	(mTissueUpperBound[i] - mTissueLowerBound[i])*HmUtil::Uniform01();

  // for simplex algorithm generate a new initial 
  // values for all process except for the master

  if (mOptimMethName == "simplex" && mRank != 0){
    srand(mRandSeed + 10*mRank);
    for (int i=0; i<mVarTissuesInitConds.size(); i++){
      mVarTissuesInitConds[i] = mTissueLowerBound[i] + 
	(mTissueUpperBound[i] - mTissueLowerBound[i])*HmUtil::Uniform01();
      mInitConds[mVarTissuesIdx[i]] =  mVarTissuesInitConds[i];
    }
  }
  return 0;
}


void CondInv::PrintInfo(ostream& outs, const string& ci){

  if (mRank == 0) {

    outs  << left; 
    outs  << "\n============== Conductivity Optimization " << endl;
    outs  << setw(25) << "Isotropic tissues: ";
    vector<string> tnames = mpPoissonSolver->GetTissueNames();
    int numTissues = tnames.size();

    for (int i=0; i<tnames.size(); i++) outs << setw(10) << tnames[i];
    
    if (mAnisotropicModel) {
      outs  << setw(25) << "skull_radial";
      outs  << setw(25) << "tang_to_radial" << endl;
    }
    outs << endl;

    outs << setw(25) << "Initial conductivities: ";
    for (int i=0; i<mInitConds.size(); i++){
      if (i<tnames.size()) outs << setw(10) << mInitConds[i];
    }

    if (mAnisotropicModel) {
	outs  << setw(25) << mInitConds[numTissues];
	outs  << setw(25) << mInitConds[numTissues+1];
    }
    outs << endl;

    outs << setw(25) << "Variables: ";
    for (int i=0; i<mVarTissuesIdx.size(); i++) {
      int tissueIdx = mVarTissuesIdx[i];
      if (tissueIdx < numTissues)
	outs <<setw(10) << tnames[tissueIdx];
      else if (mAnisotropicModel && tissueIdx == numTissues)
	outs <<setw(15) << "skull_radial";
      else if (mAnisotropicModel && tissueIdx == numTissues+1)
	outs <<setw(15) << "tang_to_radial";
    }
    outs << endl;

    outs << setw(25) << "Variables init conds: ";
    for (int i=0; i<mVarTissuesInitConds.size(); i++) {
      int tissueIdx = mVarTissuesIdx[i];
      if (tissueIdx >= numTissues && !mAnisotropicModel) 
	continue;

      outs <<setw(10) << mVarTissuesInitConds[i];
    }
    outs << endl;

    outs << setw(25) << "Variables lower bound: ";
    for (int i=0; i<mVarTissuesInitConds.size(); i++) 
      outs << setw(10) << mTissueLowerBound[i];

    outs << endl;
    
    outs << setw(25) << "Variables upper bound: ";
    for (int i=0; i<mVarTissuesInitConds.size(); i++) 
      outs << setw(10) << mTissueUpperBound[i];

    outs << endl;

    outs << setw(25) << "Objective function: " << mObjFuncMeth << endl;

    if (!ci.empty()){
      outs << setw(25) << "Current injection file: " << ci << endl;
    }

    outs << "\n============== Forward solver " << endl;
    mpPoissonSolver->PrintInfo(outs);

    outs << setw(25) << "Data path: " << mDataPath << endl;

    if (!mBrainkDataDir.empty()){
      outs << setw(25) << "Braink data dir: " << mBrainkDataDir  << endl;
      outs << setw(25) << "Data set kind: "   << mBrainkDataKind << endl;
    }
    else{
      outs << setw(25) << "Geometry file: " << mGeomFileName << endl;
      outs << setw(25) << "Sensors file: "  << mSnsFileName << endl;
    }

    outs << "\n============== Optimization method " << endl;
    if (mOptimMethName == "simplex"){
      outs << setw(25) << "Optimization algorithm: " << "Simplex" << endl;
    }
    else {
      outs << setw(25) << "Optimization algorithm: " 
	   << "Simulated annealing" << endl;
      outs << setw(25) << "Random seed: " << mRandSeed << endl;
    }
    
    if (mpOptimizer != NULL){
      mpOptimizer->PrintInfo(outs);
    }

    int size;
    MPI_Comm_size( mMpiComm, &size );

    outs << "\n============== Computation " << endl;
    outs << setw(25) << "Number of threads: " << size << endl;
    outs << setw(25) << "Accelerator: "       << mCompMode << endl;
  }
}

void CondInv::WriteResults(ostream& outs, const vector<float> &optimalConds, 
			   const vector<float> &initConds, 
			   float objFunOptimValue, 
			   int numberFcnEval, int returnCode){

  outs << setw(25) << "Initial conductivities: ";
  for (int i=0; i<initConds.size(); i++){
    if (i>= mNumIsoTissues && !mAnisotropicModel)
      continue;

    outs << setw(13) << initConds[i];
  }
  outs << endl;
  
  outs << setw(25) << "Optimal conductivities: ";
  for (int i=0; i<optimalConds.size(); i++){
    if (i>= mNumIsoTissues && !mAnisotropicModel)
      continue;
    outs << setw(13) << optimalConds[i];
  }

  outs << endl;
  outs << setw(25) << "Optimal value: " << objFunOptimValue << endl;
  outs << setw(25) << "Number of function eval: " << numberFcnEval << endl;
  outs << setw(25) << "Termination: "             << returnCode << endl;
  outs << endl;

}

void CondInv::Inverse(const string& rMeasuredFileName, ostream& outs, 
		      ostream& rProcessOutStream) {

  vector<float> optimalVarConds(mVarTissuesInitConds.begin(), 
				mVarTissuesInitConds.end());

  CurrentInjecData cid(rMeasuredFileName);
  mpObjFunc->set_measured_data(cid);

  if  (mOptimMethName == "simplex")
    mpOptimizer = new Simplex(optimalVarConds.size(), mpObjFunc, mRank);
  else if (mOptimMethName == "sa"){
    mpOptimizer = new SimAnneal(optimalVarConds.size(), 
				mpObjFunc, mMpiComm, mRank);
  }
  else
    HmUtil::ExitWithError( "IOError: not implemented optimization method " + 
			   mOptimMethName );

  mpOptimizer->SetParams(mInputParams);
  mpOptimizer->SetVarLowerBound(mTissueLowerBound); 
  mpOptimizer->SetVarUpperBound(mTissueUpperBound); 

  if (mRank == 0){
    PrintInfo(outs, rMeasuredFileName);
    outs << "\n============== Running  " << endl;
  }

  float   objFunOptimValue; 
  int     returnCode; 
  int     numberFcnEval = 0;

  returnCode    = mpOptimizer->Optimize(optimalVarConds, objFunOptimValue, 
					mpObjFunc, &rProcessOutStream);
  numberFcnEval = mpOptimizer->GetNumberFcnEval();

  // update variable tissues with optimal conds
  vector<float>  optimalConds = mInitConds;
  for (int i=0; i<optimalVarConds.size(); i++)
    optimalConds[mVarTissuesIdx[i]] = optimalVarConds[i];

  MPI_Status stat;

  if (mRank == 0){
    vector<string> tissueNames  = mpPoissonSolver->GetTissueNames();
    outs << "\n============== Results  " << endl;
    outs << setw(25) << "Tissues: ";

    for (int i=0; i<tissueNames.size(); i++){
      outs << setw(13) << tissueNames[i];
    }
    outs << endl;

    WriteResults(outs, optimalConds, mInitConds, objFunOptimValue, 
		 numberFcnEval, returnCode);
    if (mOptimMethName == "simplex") {
      int size;
      MPI_Comm_size(mMpiComm, &size);

      for (int i = 1; i<size; i++){
	MPI_Recv(&optimalConds[0], optimalConds.size(), MPI_FLOAT, 
		 i, MPI_ANY_TAG, mMpiComm, &stat);
	MPI_Recv(&mInitConds[0], mInitConds.size(), MPI_FLOAT, 
		 i, MPI_ANY_TAG, mMpiComm, &stat);
	MPI_Recv(&objFunOptimValue, 1, MPI_FLOAT, i, MPI_ANY_TAG, 
		 mMpiComm, &stat);
	MPI_Recv(&numberFcnEval, 1, MPI_INT, i, MPI_ANY_TAG, 
		 mMpiComm, &stat);
	MPI_Recv(&returnCode, 1, MPI_INT, i, MPI_ANY_TAG, 
		 mMpiComm, &stat);
	WriteResults(outs, optimalConds, mInitConds, objFunOptimValue, 
		     numberFcnEval, returnCode);
      }
    }
  }

  else {

    if (mOptimMethName == "simplex") {
      MPI_Send(&optimalConds[0], optimalConds.size(), MPI_FLOAT, 0, 0, mMpiComm);
      MPI_Send(&mInitConds[0], mInitConds.size(), MPI_FLOAT, 0, 0, mMpiComm);
      MPI_Send(&objFunOptimValue, 1, MPI_FLOAT, 0, 0, mMpiComm);
      MPI_Send(&numberFcnEval, 1, MPI_INT, 0, 0, mMpiComm);
      MPI_Send(&returnCode, 1, MPI_INT, 0, 0, mMpiComm);
    }

  }

  if (mpOptimizer) {delete mpOptimizer; mpOptimizer = NULL;}
}

void  CondInv::Optimize(string& rMeasuredData) {

  if (mMyTask == NOTHING) return;

  // get full path name 

  HmUtil::TrimStrSpaces(rMeasuredData);
  if (rMeasuredData[0] != '/'){
   rMeasuredData  = mDataPath +"/"+ rMeasuredData;
  }

  bool isd = HmUtil::IsDirectory(&rMeasuredData[0]);
 
  string            eitFileExt = ".ci", eitFileName;
  int               pos;
  
  if (!isd){ //processing single
    OptimizeFile(rMeasuredData) ;
  }

  else{     //processing a folder of files 
    vector<string> eitFiles;
    if (mRank == 0){
      cout << "Processing directory .... " << rMeasuredData << endl;
    }

    //    getdir(rMeasuredData, eitFiles);
    HmUtil::GetFilesInDirectory(rMeasuredData, eitFiles);

    if (eitFiles.empty()){
      if (mRank == 0){
	HmUtil::ExitWithError( "IOError: no current injection data files to process " );
      }
      else exit(1);
    }

    for (int i=0; i<eitFiles.size(); i++){
      int fsize = eitFiles[i].size();

      // Check to see if the EIT files end with the right extension:
      if ( (pos=fsize-eitFileExt.size())>0 && eitFiles[i].substr(pos)==eitFileExt){
	      eitFileName = rMeasuredData + "//" + eitFiles[i];
	      if (mRank == 0){
	        cout << "Processing file: " << eitFiles[i]  << endl;
	      }
	OptimizeFile(eitFileName);
      }
      else {
	if (mRank == 0 && eitFiles[i]!= "." && eitFiles[i]!=".."){
	  cout << "WARNING: skipping file: " << eitFiles[i]  
	       << " because it has the wrong extension" << endl;
	}
      }
    }
  }
}

void  CondInv::OptimizeFile(const string& rMeasuredData) {

  //output file that shows the convergence
  string     optimizationConvFile = HmUtil::ExtractFileName(rMeasuredData) + "o";
  string optimizationConvFileProc = optimizationConvFile + 
    "_" + HmUtil::IntToString(mRank);

  ofstream            outs;
  ofstream            processOutStream;

  /*
  if (FileExists(optimizationConvFile)){
    if (mRank == 0){
      cout << "Output file exists ... nothing done ... skipped " <<  optimizationConvFile << endl;
      return;
    }
  }
  */

  MPI_Barrier(mMpiComm);
  processOutStream.open(&optimizationConvFileProc[0]);

  if (!processOutStream.is_open()){
    HmUtil::ExitWithError( "IOError: can not open output file " + optimizationConvFileProc );
  }

  if (mRank == 0){
    cout << "Processing file: " << rMeasuredData << endl;
    outs.open(&optimizationConvFile[0]);
    if (!outs.is_open()){
      HmUtil::ExitWithError( "IOError: can not open output file " + optimizationConvFile );
    }
      
    Inverse(rMeasuredData, outs, processOutStream);
    outs.close();
  }

  else{
    Inverse(rMeasuredData, cout, processOutStream);
  }
  processOutStream.close();
}
