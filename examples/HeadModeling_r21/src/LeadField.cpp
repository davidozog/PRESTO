////////////////////////////////////////////////////////////////////
///// Implementation file for lfm class                /////////////
///// NeuroInformatic Center -- adnan@cs.uoregon.edu   /////////////
////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <cctype>
#include <sys/time.h>

#include <iostream>
#include <fstream>

#include <omp.h>
#include <mpi.h>

#include <algorithm>
#include <cmath>

#include "LeadField.h"
#include "HmUtil.h"
#include "PoissonAdi.h"
#include "PoissonVai.h"

using std::cout;
using std::endl;
using std::string;
using std::cerr;
using std::setw;
using std::ofstream;
using std::ifstream;
using std::ios;
using std::setfill;
using std::streampos;

const int MAX_TASKS_PER_NODE = 2;

LeadField::LeadField(){
  Init();
};

LeadField::LeadField(map<string, vector<string> > params){ Init(params);}

int LeadField::Init(){

  MPI_Comm_rank( MPI_COMM_WORLD, &mRank );

  mDataPath                       = "./";
  mOutputNamePrefix               = "";
  mDipoleMomentMagAm              = 0.001;
  mUseReciprocity                 = true;
  mUseTriples                     = false;
  mRefElectrodeId                 = -1;
  mCurrent                        = 1.0;
  mParallelism                    = "omp";
  mDviceOrThreads                 = 0;
  mMyTask                         = NOTHING;
  mSwapBytes                      = false;
  mUnitStandard                   = "physics_am2v";
  mLfmFileNameTrip                = ""; 
  mLfmFileNameOri                 = "";
}

int LeadField::Init(map<string, vector<string> > params){

  Init();

  string  refSensor                   = "";
  string  algorithm                   = "";

  float   skullRadialCond             = -1;
  float   tangentialToRadialRatio     = 1;

  float   convEps                     = 0.005;
  int     convCheck                   = 5;
  float   tolerance                   = 0.001; 
  float   timeStep                    = 2.8; 
  int     maxNumIterations            = 2000;
  string  lfmEndian                   = "auto";

  string         param;
  vector<string> pvalue, inTissueNames, inTissueConds;

  map<string, vector<string> >::const_iterator iter;

  for (iter = params.begin(); iter != params.end(); iter++){
  
    param = iter->first; pvalue = iter->second;

    if (param == "datapath") {
      if (!pvalue[0].empty()) mDataPath = pvalue[0];
    }

    //TODO: move parsing forward solver parameters to Poisson class 
    //      or make a parser class 

    else if (param == "algorithm")    algorithm = pvalue[0]; 
    else if (param == "parallelism")        mParallelism  = pvalue[0]; 
    else if (param == "convergence_check")  convCheck = atoi(&pvalue[0][0]);
    else if (param == "convergence_eps")    convEps = atof(&pvalue[0][0]);
    else if (param == "output_name_prefix") mOutputNamePrefix = pvalue[0];
    else if (param == "skull_normal_cond")     skullRadialCond     = atof(&pvalue[0][0]);
    else if (param == "tang_to_normal_ration") tangentialToRadialRatio = atof(&pvalue[0][0]);
    else if (param == "dipole_moment_mag") mDipoleMomentMagAm = atof(&params["dipole_moment_mag"][0][0]);
    else if (param == "refSensor")  refSensor = pvalue[0];
    else if (param == "tol")  tolerance = atof(&pvalue[0][0]);
    else if (param == "time_step")  timeStep = atof(&pvalue[0][0]);
    else if (param == "max_iter")  maxNumIterations = atoi(&pvalue[0][0]);
    else if (param == "tissues") inTissueNames = params["tissues"];
    else if (param == "tissues_conds") inTissueConds = params["tissues_conds"];
    else if (param == "use_reciprocity") mUseReciprocity = (bool) atoi(&pvalue[0][0]);
    else if (param == "use_triples") mUseTriples = (bool) atoi(&pvalue[0][0]);
    else if (param == "unit_standard") {
      
      string unitStandard = pvalue[0];
      HmUtil::TrimStrSpaces(unitStandard);
      if ( unitStandard.empty() )  unitStandard = "physics_am2v";

      if ( unitStandard != "physics_am2v" && unitStandard != "psychophysiology_nam2microv" )
	HmUtil::ExitWithError("Unrecognized unit standard: " + unitStandard );
     
      mUnitStandard = unitStandard;
    }

    else if (param == "lfm_endianness") {
      lfmEndian = pvalue[0];
      HmUtil::TrimStrSpaces(lfmEndian);
      if (lfmEndian.empty()) lfmEndian = "auto";
      if (lfmEndian != "auto" && lfmEndian != "little" && lfmEndian != "big"){
	HmUtil::ExitWithError("Unrecognized endanness : " + lfmEndian );
      }
    }
  }

  bool isLittle = HmUtil::IsLittleEndian();
  mSwapBytes = isLittle && (lfmEndian == "big") || 
    !isLittle && (lfmEndian == "little");

  HmUtil::TrimStrSpaces(algorithm);
  HmUtil::TrimStrSpaces(mParallelism);

  MyTask(mParallelism, mDviceOrThreads, mMyTask);

  if (mMyTask == NOTHING) return 0;
  if (mOutputNamePrefix.empty()) mOutputNamePrefix = "lfm_matrix";

  if (mDataPath[mDataPath.size()-1] != '/') mDataPath += "/";

  if (algorithm == "vai")       P = new PoissonVAI();
  else if (algorithm == "adi")  P = new PoissonADI();
  else  HmUtil::ExitWithError("IOError: Poisson, unsupported forward algorithm ... " + algorithm);

  mHeadModel.Init(params);

  if (mHeadModel.mDipolesPosMap.empty())
    HmUtil::ExitWithError("DataError: no dipoles position specified ... ");


  // When using reciprocity we can compute the oriented dipoles LFM
  // without computing the tripls which is 1/3 of using the triples.
  // But without reciprocity, puting the two monopoles on a line aligned 
  // with orientation of the dipole is harder. .   
  // TODO make more clear 

  if (mUseReciprocity){
    if (mUseTriples) {
      SetDipolesTriples();    // if the triples are requested then use them
      mLfmFileNameTrip = mOutputNamePrefix + "_trip" + ".gs";
      if (!mHeadModel.mDipolesOrientationMap.empty())
	mLfmFileNameOri = mOutputNamePrefix + "_ori" + ".gs";

    }

    else if (mHeadModel.mDipolesOrientationMap.empty()){
      mUseTriples = true;
      cout << "Info: No dipoles orientations specified ... using dipole triplets " << endl;
      SetDipolesTriples();
      mLfmFileNameTrip = mOutputNamePrefix + "_trip" + ".gs";
    }
      
    else {
      mUseTriples = false;
      SetDipoleMoments();
      mLfmFileNameOri = mOutputNamePrefix + "_ori" + ".gs";
    }
  }
 
  else{
    mUseTriples = true;
    SetDipolesTriples();
    mLfmFileNameTrip = mOutputNamePrefix + "_trip" + ".gs";
    if (!mHeadModel.mDipolesOrientationMap.empty())
      mLfmFileNameOri = mOutputNamePrefix + "_ori" + ".gs";
  }

  //  if (mRank == mMaster){
  //    cout << "mLfmFileNameTrip: " << mLfmFileNameTrip << endl;
  //    cout << "mLfmFileNameOri: " <<  mLfmFileNameOri << endl;
  //  }

  if (mMyTask == MASTER) return 0;
 
  if(P->SetHeadModel(mHeadModel))
    HmUtil::ExitWithError("Error: Poisson, error setting geometry, sensors or dipoles ... ");

  SetReferenceSensor(refSensor);

  // TODO move setting forward parameters to Poisson class
  //      and pass parameters map

  P->SetTangentToNormalRatio(tangentialToRadialRatio);
  P->SetSkullNormalCond(skullRadialCond);
  P->init();

  for (int i=0; i<inTissueNames.size() && i<inTissueConds.size(); i++)
    P->SetTissueConds(inTissueNames[i], atof(&inTissueConds[i][0]));

  P->SetConvTolerance(tolerance);
  P->SetTimeStep(timeStep);
  P->SetMaxIterations(maxNumIterations);
  P->SetConvEps(convEps);
  P->SetConvCheck(convCheck);

  return 0;
}


// Prints information about the LFM in a header file 

int LeadField::PrintHeader(){

  string lfmHeaderFileName = mOutputNamePrefix + ".hdr" ;
  ofstream outs(&lfmHeaderFileName[0]);

  outs << setiosflags( ios::left );
  outs << "\n=========== LFM Generation " << endl;

  mHeadModel.PrintInfo(outs);

  outs << setw(25) << "Using triples: ";
  mUseTriples ? outs << "True" << endl : outs << "False" << endl;

  outs << setw(25) << "Using reciprocity: ";
  mUseReciprocity ? outs << "True" << endl :outs  << "False" << endl;

  if (mRefElectrodeId > 0){
    outs << setw(25) << "Reference electrode id: " << mRefElectrodeId << endl;
  }

  outs << setw(25) << "Dipole moment magnitude: " << mDipoleMomentMagAm << " A.m " << endl;

  bool isLittle = HmUtil::IsLittleEndian();
  bool big = isLittle && mSwapBytes || !isLittle && !mSwapBytes;
  outs << setw(25) << "LFM endianness: " << (big ? "big" : "little") << endl;
  outs << setw(25) << "LFM unit standard: " << mUnitStandard << endl;
  
  if (!mLfmFileNameTrip.empty()){
    outs << setw(25) << "Triple LFM file name: " << mLfmFileNameTrip << endl;
    outs << setw(25) << "Triple LFM dimension:" <<  "[" << mHeadModel.mSensorsMap.size()  << "," 
		 <<  mDipolesMap.size() << "]" << endl;
  }
  if (!mLfmFileNameOri.empty()) {
    outs << setw(25) << "Oriented LFM file name: " << mLfmFileNameOri << endl;
    outs << setw(25) << "Oriented LFM dimension:" <<  "[" << mHeadModel.mSensorsMap.size()  << "," 
		 <<  mHeadModel.mDipolesPosMap.size() << "]" << endl;
  }

  outs << setw(25) << "LFM format: " << "GeoSource" << endl;
  outs << endl;

  outs << "=========== Forward solver parameters " << endl;
  P->PrintInfo(outs);
  outs << endl;

  outs << "=========== Accelerator " << endl;
  outs << setw(25) << "Parallelism: " << mParallelism << endl;
  outs.close();

}

// returns the sensor ID to be used as a reference sensor
// INPUT: choice of reference electrode 
//        1) Cz as labeled in the sensors file
//        2) Sensors ID: the id of a sensors specified in the sensors file
//        3) anything else: the function returns the sensors on the top of the head
//           the sensors with largest z value


int LeadField::SetReferenceSensor(string& refElectrode){

  if (refElectrode == "Cz" || refElectrode.empty() || !HmUtil::IsNumber(refElectrode) ){
    map<string, vector<int> >::iterator loc = mHeadModel.mRefSensorsMap.find("Cz");
    if (loc != mHeadModel.mRefSensorsMap.end()){ // we have a Cz sensor
      mRefElectrodeId  = -1; // not a data sensor
      mRefElectrodePos = loc->second;
    }
    else{   // not Cz, find the sensor on the top of the head
      map<int, vector<int> >::const_iterator it = mHeadModel.mSensorsMap.begin();
      mRefElectrodeId = it->first;
      
      for (;  it != mHeadModel.mSensorsMap.end(); it++){
	if (it->second[2] > mHeadModel.mSensorsMap[mRefElectrodeId][2]) {
	  mRefElectrodeId = it->first;
	}
      }
      mRefElectrodePos =  mHeadModel.mSensorsMap[mRefElectrodeId];
    }
  }

  else { // use a data sensor as requested 
    mRefElectrodeId = atoi(&refElectrode[0]);
    if (mHeadModel.mSensorsMap.find(mRefElectrodeId) == mHeadModel.mSensorsMap.end())
      HmUtil::ExitWithError("DataError: bad reference electrode choice ");
    else
      mRefElectrodePos = mHeadModel.mSensorsMap[mRefElectrodeId];
  }   
  return 0;
}

// TODO: get rid of this method and compute dipole moment on the fly
// Given a dipoles orientations data, this method sets the dipole moments 

int LeadField::SetDipoleMoments(){

  const map<int, vector<int> >&   dipolesPosMap    = mHeadModel.mDipolesPosMap;
  const map<int, vector<float> >& dipolesOrientMap = 
    mHeadModel.mDipolesOrientationMap;

  int numDipoles       = dipolesPosMap.size();
  int numOrientDipoles = dipolesOrientMap.size();

  if (numDipoles != numOrientDipoles) {
    HmUtil::ExitWithError(string("DataError: number dipoles different from ") + 
		    string("number oriented dipoles ") +
		    HmUtil::IntToString(numDipoles) + 
		    HmUtil::IntToString(numOrientDipoles) );
  }

  int id, oid;  //position and orientation

  map<int, vector<int> >::const_iterator dip_pos_iter = dipolesPosMap.begin();
  map<int, vector<float> >::const_iterator  dip_orient_iter = dipolesOrientMap.begin();

  for (int i=0; i<numDipoles; i++){
    id = dip_pos_iter->first;
    int x  = dip_pos_iter->second[0];  // Position
    int y  = dip_pos_iter->second[1];
    int z  = dip_pos_iter->second[2];

    oid = dip_orient_iter->first;      // Orientation
    float ox  = dip_orient_iter->second[0];
    float oy  = dip_orient_iter->second[1];
    float oz  = dip_orient_iter->second[2];

    if (id != oid){
      HmUtil::ExitWithError("DataError: Inconsistent dipole positions and orientations " +
		      HmUtil::IntToString(id) + HmUtil::IntToString(oid));
    }

    mDipolesMap[id].push_back((float) x); 
    mDipolesMap[id].push_back((float) y); 
    mDipolesMap[id].push_back((float) z); 
    mDipolesMap[id].push_back(ox); 
    mDipolesMap[id].push_back(oy); 
    mDipolesMap[id].push_back(oz);

    dip_orient_iter++;
    dip_pos_iter++;
  }

  return 0;
}

// TODO: Get rid of this method and compute the triples on the fly in the computation
// will save memory

// This methods sets the dipole moments triples along x, y, and z direction
int LeadField::SetDipolesTriples(){

  const map<int, vector<int> >&  dipolesMap = mHeadModel.mDipolesPosMap;

  for (map<int, vector<int> >::const_iterator it = dipolesMap.begin(); 
       it!= dipolesMap.end(); it++){

    int dipoleId = it->first;
    int x = it->second[0];
    int y = it->second[1];
    int z = it->second[2];
    
    int dipoleCompId = dipoleId * 10 + 1;
    mDipolesMap[dipoleCompId].push_back(x); 
    mDipolesMap[dipoleCompId].push_back(y); 
    mDipolesMap[dipoleCompId].push_back(z); 
    mDipolesMap[dipoleCompId].push_back(1); 
    mDipolesMap[dipoleCompId].push_back(0); 
    mDipolesMap[dipoleCompId].push_back(0); 
    
    dipoleCompId = dipoleId * 10 + 2;
    mDipolesMap[dipoleCompId].push_back(x); 
    mDipolesMap[dipoleCompId].push_back(y); 
    mDipolesMap[dipoleCompId].push_back(z);
    mDipolesMap[dipoleCompId].push_back(0); 
    mDipolesMap[dipoleCompId].push_back(1); 
    mDipolesMap[dipoleCompId].push_back(0); 
      
    dipoleCompId = dipoleId * 10 + 3;
    mDipolesMap[dipoleCompId].push_back(x); 
    mDipolesMap[dipoleCompId].push_back(y); 
    mDipolesMap[dipoleCompId].push_back(z);
    mDipolesMap[dipoleCompId].push_back(0); 
    mDipolesMap[dipoleCompId].push_back(0); 
    mDipolesMap[dipoleCompId].push_back(1);
  }

  return 0;
}

// This method updates the progress time on the screen

void LeadField::UpdateProgress(int id, int recievedFrom, int total, 
			       double stime, const string& mode){

  double rtime = HmUtil::GetWallTime();
  static int done = 0;
  done++;

  int tleft = (int) (((rtime - stime)/done) * (total - done));
  int hour = tleft/3600; 
  int tremain = tleft%(3600);
  int min = tremain/60; 
  int sec = tremain%60;
     
  cout <<mode << " = " << id << " :: " << "from " << recievedFrom 
       << " :: " << "done " << done  << "/" <<  total << " :: time left = "; 
  cout << setfill('0') << setw(2) << hour << ":" << setw(2) << min 
       << ":" << setw(2) << sec << endl;
}


// This method is called only by the MASTER process 
// to get a list of the processes that are capable of doing work -- joined the pool of workers
// Note: not all processes do work 
// The method return a list of the ranks of the processes that joined the pool

vector<int>  LeadField::GetWorkersRanks(){
  
  int size;
  MPI_Comm_size( MPI_COMM_WORLD, &size );

  MPI_Status stat;
  vector<int> workersRanks;
  int join;

  //  workersRanks.push_back(0);

  //FIXIT:  Use MPI_Gather instead, more readable 
  for (int i=1; i<size; i++){
    MPI_Recv(&join, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
    if (join == 1)
      workersRanks.push_back(stat.MPI_SOURCE);
  }
  return workersRanks;
}

// Writes the potentials at a particular sensor due to every dipole 
// This method is called only by MASTER when using reciprocity
// since each worker in this case computes the potentails at one sensors
// corresponding to every dipole 

//TODO: remove redundant num_dipoles (= rdata.size() -1) argument 
void LeadField::WriteSensorField(ostream& outs, int sensor, 
				 int num_dipoles, vector<float>& rdata){

  streampos spos = (streampos) sensor*(num_dipoles)*sizeof(double);
  outs.seekp(spos, ios::beg);

  // TODO: write in chunck instead of elements 
  for (int i=1; i<rdata.size(); i++){
    double yy = (double) rdata[i];
    outs.write((char*) &yy, sizeof(double));
  } 
  
}

// Writes the potentials at all sensors corresponding to a single dipole 
// The method is called only by MASTER when using forward calculation 
// since each worker computes the potentials at all sensors for a dipole

//TODO: get rid of redundant argument numDipoles (=rdata.size() - 1)
void LeadField::WriteDipoleField(ostream& outs, int dipole, 
				 int numDipoles, vector<float>& rdata){

  outs.seekp((streampos) dipole*sizeof(double), ios::beg);
  
  for (int i=1; i<rdata.size(); i++){
    double yy = (double) rdata[i];
    outs.write((char*) &yy, sizeof(double));
    outs.seekp((numDipoles-1)*sizeof(double), ios::cur);
  }
}

// This method called by the MASTER that distribute work on the worker
void LeadField::MasterReciprocity(){

  double  stime = HmUtil::GetWallTime();
  string  lfmFileName;

  // TODO: move file naming in initializtion 
  //  if (mUseTriples) lfmFileName =  mOutputNamePrefix + "_trip" + ".gs";
  //  else lfmFileName = mOutputNamePrefix + "_ori" + ".gs";
  //  mLfmFileNameTrip  mLfmFileNameOri

  if (!mLfmFileNameTrip.empty()) lfmFileName = mLfmFileNameTrip;
  else if (!mLfmFileNameOri.empty() )lfmFileName = mLfmFileNameOri;
  else HmUtil::ExitWithError("IOError: Lfm file name ");

  ofstream lfmsb(&lfmFileName[0], ios::binary | ios::out);
  if (!lfmsb.is_open())  HmUtil::ExitWithError("Error: Can't open binary file ... ");

  MPI_Status stat;
  vector<int> workersRanks = GetWorkersRanks();

  if (workersRanks.empty()) 
    HmUtil::ExitWithError("No workers available ");

  // ask one active worker to print the header file 
  // note: master doesn't load Poisonn object 
  int printHeader = 1;
  MPI_Send(&printHeader, 1, MPI_INT, workersRanks[0], 0, MPI_COMM_WORLD);

  printHeader = 0;
  for (int i=1; i<workersRanks.size(); i++){
    MPI_Send(&printHeader, 1, MPI_INT, workersRanks[i], 0, MPI_COMM_WORLD);
  }
  ///

  vector<int> sensorsIds;
  map<int, vector<int> >::iterator iter = mHeadModel.mSensorsMap.begin();
  for(; iter != mHeadModel.mSensorsMap.end(); iter++) 
    sensorsIds.push_back(iter->first);
    
  int i=0, j=0;
  for (; i<workersRanks.size() && j<sensorsIds.size(); i++,j++){
    MPI_Send(&j, 1, MPI_INT, workersRanks[i], 0, MPI_COMM_WORLD);
  }

  // Make sure doesn't cause a bug when number of dipoles is 
  // less than number of workers
  int numActiveWorkers = i;

  int numSensors = sensorsIds.size();
  int numDipoles = mDipolesMap.size();

  vector<float> rdata(numDipoles+1, 0);
  int recvd_from, sensor;

  for (; j<sensorsIds.size(); j++){

    MPI_Recv(&rdata[0], numDipoles + 1, MPI_FLOAT, MPI_ANY_SOURCE, 
	     MPI_ANY_TAG, MPI_COMM_WORLD, &stat);

    recvd_from = stat.MPI_SOURCE; 
    sensor = (int) rdata[0];

    UpdateProgress(sensor, recvd_from, numSensors, stime, "Sensor");
    WriteSensorField(lfmsb, sensor, numDipoles, rdata);

    MPI_Send(&j, 1, MPI_INT, recvd_from, 0, MPI_COMM_WORLD);
  }

  for (int i=0; i<numActiveWorkers; i++){
    MPI_Recv(&rdata[0], numDipoles+1, MPI_FLOAT, MPI_ANY_SOURCE, 
	     MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
    recvd_from = stat.MPI_SOURCE; 
    
    sensor = (int) rdata[0];
    UpdateProgress(sensor, recvd_from, numSensors, stime, "Sensor");
    WriteSensorField(lfmsb, sensor, numDipoles, rdata);
  }
    
  int finish = -1;
  for (int i=0; i<workersRanks.size(); i++)
    MPI_Send(&finish, 1, MPI_INT, workersRanks[i], 0, MPI_COMM_WORLD);

  lfmsb.close();
}

void LeadField::MasterForward(){

  double stime = HmUtil::GetWallTime();

  string lfmFileNameTrip = mOutputNamePrefix + "_trip" + ".gs";
  if (!mLfmFileNameTrip.empty()) lfmFileNameTrip = mLfmFileNameTrip;
  else HmUtil::ExitWithError("IOError: Lfm file name ");

  ofstream lfmsb(&lfmFileNameTrip[0], ios::binary | ios::out);
  if (!lfmsb.is_open())
    HmUtil::ExitWithError("Error: Can't open binary file ... " + lfmFileNameTrip);

  MPI_Status stat;
  vector<int> workersRanks = GetWorkersRanks();

  if (workersRanks.empty()) 
    HmUtil::ExitWithError("No workers available ");
  
  // ask one active worker to print the header file 
  // note: master doesn't load Poisonn object 
  int printHeader = 1;
  MPI_Send(&printHeader, 1, MPI_INT, workersRanks[0], 0, MPI_COMM_WORLD);

  printHeader = 0;
  for (int i=1; i<workersRanks.size(); i++){
    MPI_Send(&printHeader, 1, MPI_INT, workersRanks[i], 0, MPI_COMM_WORLD);
  }
  ///

  vector<int> dips_ids;
  for(map<int, vector<float> >::iterator it = mDipolesMap.begin();
      it != mDipolesMap.end(); it++)
    dips_ids.push_back(it->first);
  
  vector<int>::iterator iter = dips_ids.begin();
  int i = 0, j = 0;

  for (; i<workersRanks.size() && j< dips_ids.size(); i++, j++)
    MPI_Send(&j, 1, MPI_INT, workersRanks[i], 0, MPI_COMM_WORLD);

  int num_active_workers = i;
  int num_sensors = mHeadModel.mSensorsMap.size();
  int num_dipoles = mDipolesMap.size();

  vector<float> rdata(num_sensors+1, 0);
  vector<float>::iterator it;

  int dipole, recvd_from; // recvd_tag;

  for (; j<dips_ids.size(); j++){

    MPI_Recv(&rdata[0], num_sensors+1, MPI_FLOAT, MPI_ANY_SOURCE, 
	     MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
    recvd_from = stat.MPI_SOURCE; //recvd_tag = stat.MPI_TAG; 

    dipole = (int) rdata[0];
    UpdateProgress(dipole, recvd_from, num_dipoles, stime, "Dipole");
    WriteDipoleField(lfmsb, dipole, num_dipoles, rdata);
    MPI_Send(&j, 1, MPI_INT, recvd_from, 0, MPI_COMM_WORLD);
  }

  for (int i=0; i<num_active_workers; i++){
    MPI_Recv(&rdata[0], num_sensors+1, MPI_FLOAT, MPI_ANY_SOURCE, 
	     MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
    recvd_from = stat.MPI_SOURCE; // recvd_tag = stat.MPI_TAG;
    dipole = (int) rdata[0];
    UpdateProgress(dipole, recvd_from, num_dipoles, stime, "Dipole");
    WriteDipoleField(lfmsb, dipole, num_dipoles, rdata);
  }
    
  int flags = -1;
  for (int i=0; i<workersRanks.size(); i++)
    MPI_Send(&flags, 1, MPI_INT, workersRanks[i], 0, MPI_COMM_WORLD);

  lfmsb.close();
}


void LeadField::WorkerReciprocity( const string& mode, int threads ){

  MPI_Status stat;  
  stat.MPI_TAG = 1; 

  int printHeader;
  MPI_Recv(&printHeader, 1, MPI_INT, mMaster, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
  if (printHeader) {
    PrintHeader();
    //    cout << "Rank: " << mRank << "  " << printHeader << endl;
  }

  int electrode;
  vector<int> dpos(3, 0);
  float dori[3]    = {0};

  vector<int> sens_ids;
  for(map<int, vector<int> >::const_iterator it = mHeadModel.mSensorsMap.begin();
      it != mHeadModel.mSensorsMap.end(); it++)
    sens_ids.push_back(it->first);

  vector<float> sol(mDipolesMap.size()+1, 0);
  map<int, vector<float> >::iterator diter;

  while (1){
    MPI_Recv(&electrode, 1, MPI_INT, mMaster, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
    if (electrode < 0) break;

    sol[0] = electrode;

    if (sens_ids[electrode] != mRefElectrodeId ){

      P->reinit();
      P->RemoveCurrentSource("all");
      P->AddCurrentSource(sens_ids[electrode], mCurrent);
      P->AddCurrentSource(&mRefElectrodePos[0], -mCurrent);
      
      int num_iter = P->Solve(const_cast<string&>(mode), threads);
      //      cout << "Rank, numIteration: " << mRank << "  " << num_iter << endl;
      int kk = 1;
      vector<float> E;
      
      for (diter = mDipolesMap.begin(); diter != mDipolesMap.end(); diter++){

	dpos[0] = diter->second[0];
	dpos[1] = diter->second[1];
	dpos[2] = diter->second[2];

	dori[0] = diter->second[3]; 
	dori[1] = diter->second[4];
	dori[2] = diter->second[5];

	E = P->VoxelElectricField(dpos);
	sol[kk] = - (E[0]* dori[0] + E[1]*dori[1] + E[2]*dori[2])*mDipoleMomentMagAm ;
	kk++;
      }
    }
    else
      fill (sol.begin()+1, sol.end(), 0);

    MPI_Send(&sol[0], sol.size(), MPI_FLOAT, mMaster, stat.MPI_TAG, MPI_COMM_WORLD);
  }
}

void LeadField::WorkerForward(const string& mode, int threads){

  int source[3]  = {0}, sink[3] = {0}, k=1, dipole; 
  MPI_Comm_rank( MPI_COMM_WORLD, &mRank );

  vector<int> dips_ids;
  for(map<int, vector<float> >::const_iterator it = mDipolesMap.begin();
      it != mDipolesMap.end(); it++)
    dips_ids.push_back(it->first);
  
  MPI_Status stat;  stat.MPI_TAG = 1; 

  int printHeader;
  MPI_Recv(&printHeader, 1, MPI_INT, mMaster, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
  if (printHeader) {
    PrintHeader();
    //    cout << "Rank: " << mRank << "  " << printHeader << endl;
  }

  vector<float> sol(mHeadModel.mSensorsMap.size()+1, 0);
  map<int, float>::iterator iter1;
  float   ref_pot; 
  int     num_iter;
  map<int, float> elec_pot;
  
  // get voxel length in each direction in mm
  vector<float> voxelSizeMeter = mHeadModel.GetVoxelSizeMm();
  for (int i=0; i<3; i++) voxelSizeMeter[i] /= 1000;
  float monopolesDistanceMeter[3] = {0};


  while (1){
    MPI_Recv(&dipole, 1, MPI_INT, mMaster, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
    if (dipole < 0) break;

    sol[0] = dipole;

    P->reinit();
    P->RemoveCurrentSource("all");

    for (int i=0; i<3; i++) {
      sink[i] = (int) mDipolesMap[dips_ids[dipole]][i] + (int) mDipolesMap[dips_ids[dipole]][i+3]; 
      source[i]   = (int) mDipolesMap[dips_ids[dipole]][i] - (int) mDipolesMap[dips_ids[dipole]][i+3];

      // distance = numberOfVoexels * lengthOfVoexlMeter
      monopolesDistanceMeter[i] =  voxelSizeMeter[i] * abs(sink[i]-source[i]); 
    } 

    float dipoleDistanceMeter = sqrt(  monopolesDistanceMeter[0] * monopolesDistanceMeter[0] +  
                                       monopolesDistanceMeter[1] * monopolesDistanceMeter[1] +
                                       monopolesDistanceMeter[2] * monopolesDistanceMeter[2] );

    P->AddCurrentSource(source, mCurrent);
    P->AddCurrentSource(sink,  -mCurrent);
      
    num_iter = P->Solve(const_cast<string&>(mode), threads);
    elec_pot = P->SensorsPotentialMap();
    ref_pot = P->VoxelPotential(mRefElectrodePos);

    //    cout << "Rank, numIteration: " << mRank << "  " << num_iter << endl;

    for (k=1, iter1 = elec_pot.begin(); iter1 != elec_pot.end(); iter1++, k++) {
      sol[k] = (iter1->second - ref_pot) * (mDipoleMomentMagAm/dipoleDistanceMeter);    
    }

    MPI_Send(&sol[0], sol.size(), MPI_FLOAT, mMaster, stat.MPI_TAG, 
	     MPI_COMM_WORLD);
  }
}


// This function loads the computed triple LFM and multiply 
// then scales the potential by the dipole moment direction 
   
void LeadField::WriteOrientedLfm(){

  ifstream lfmts(&mLfmFileNameTrip[0], ios::binary | ios::in);

  if (!lfmts.is_open())
    HmUtil::ExitWithError("Error: Can't open binary file ... " + mLfmFileNameTrip);
  
  ofstream lfmos(&mLfmFileNameOri[0], ios::binary | ios::out);
  if (!lfmos.is_open())
    HmUtil::ExitWithError("Error: Can't open output binary file ... " + 
			  mLfmFileNameOri);

  bool changeUnits =  (mUnitStandard == "psychophysiology_nam2microv");
  float changeUnitsFact = 0.001; 

  // TODO: read and write by chunks to avoid 
  // memory problem when having large LFMs

  int lfmSize  = mHeadModel.mSensorsMap.size() * mDipolesMap.size();
  double * lfmTriples = new double[lfmSize];
  lfmts.read((char*) lfmTriples, lfmSize * sizeof(double));

  int k = 0;
  while (k<lfmSize){
    for (map<int, vector<float> >::const_iterator it = 
	   mHeadModel.mDipolesOrientationMap.begin();
	 it !=  mHeadModel.mDipolesOrientationMap.end() && k<lfmSize; 
	 it++, k+=3 ) {

      double sens_pot = lfmTriples[k] * it->second[0] +  
	lfmTriples[k+1] * it->second[1] +  lfmTriples[k+2] * it->second[2];
      // switch endiannes 
      if (changeUnits) sens_pot = sens_pot*changeUnitsFact;
      if (mSwapBytes) sens_pot = HmUtil::SwapBytesDouble(sens_pot);
      lfmos.write((char*) &sens_pot, sizeof(double));
    }
  }

  delete [] lfmTriples;
}

// This function switch the computed lfm endanness
void LeadField::LfmPostProcess(const string& lfmFileName) {

  fstream lfmts(&lfmFileName[0], ios::binary | ios::in | ios::out );
  lfmts.seekp(0, ios::beg);

  if (!lfmts.is_open())
    HmUtil::ExitWithError("Error: Can't open binary file ... " + lfmFileName);

  double sens_pot;

  bool changeUnits =  (mUnitStandard == "psychophysiology_nam2microv");
  float changeUnitsFact = 0.001; 

  // TODO consider effeciency by reading blocks 
  while ( lfmts.good() ) {
    int pos = lfmts.tellg();
    lfmts.read((char*) &sens_pot, sizeof(double));
    if (changeUnits) sens_pot = sens_pot*changeUnitsFact;
    if (mSwapBytes) sens_pot = HmUtil::SwapBytesDouble(sens_pot);
    lfmts.seekg(pos, ios::beg);
    lfmts.write((char*) &sens_pot, sizeof(double));
  }
  lfmts.close();
}


// TODO: move common stuff that is used also in conductivity in a seperate function for reuse 

// This method is called by all processes in the COMM_WORLD, it returns 
// the parallism to be used. In case of CUDA it returns the device ID and in case of OMP
// it returns the number of threads to be used. Also, the method returns 
// wheather the process is a MASTER, WORKER, DONOTHING
 
// IN/OUT: parallelism          [omp, cuda, auto] 
// OUT   : deviceIdOrNumThreads
// OUT   : myTask               [MASTER, WORKER, NOTHING]
 
void LeadField::MyTask(string& parallelism, int& deviceIdOrNumThreads, ProcessTask& myTask){

  // first all processes communicate the hostnames
  // so each process know which processes shares the node

  int      nprocs, namelen, rank;

  char     host_name[MPI_MAX_PROCESSOR_NAME];
  char (*host_names)[MPI_MAX_PROCESSOR_NAME];

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Get_processor_name(host_name,&namelen);

  int bytes = nprocs * sizeof(char[MPI_MAX_PROCESSOR_NAME]);
  host_names = (char (*)[MPI_MAX_PROCESSOR_NAME]) malloc(bytes);
 
  strcpy(host_names[rank], host_name);
  MPI_Allgather(&(host_names[rank]), MPI_MAX_PROCESSOR_NAME, MPI_CHAR, host_names, 
		MPI_MAX_PROCESSOR_NAME, MPI_CHAR,  MPI_COMM_WORLD);

  /*  
  // TODO: replace with MPI_Allgather instead 
  for (int n=0; n<nprocs; n++) {
    MPI_Bcast(&(host_names[n]),MPI_MAX_PROCESSOR_NAME, MPI_CHAR, n, MPI_COMM_WORLD); 
  }
  */

  map<string, vector<int> > nodes;

  for (int i=0; i<nprocs; i++){
    string rankNode(host_names[i]);
    HmUtil::TrimStrSpaces(rankNode);
    nodes[rankNode].push_back(i);
  }

  // sort them, now every process have a list of the process on its own node
  // sorted according to their ranks 
  map<string, vector<int> >::iterator it = nodes.begin();
  for (; it != nodes.end(); it++){
    sort((it->second).begin(), (it->second).end());
  }

  // Who is the MASTER 
  // The lowest rank in the node with the largest number of ranks is to be 
  // the MASTER
  int     master           = 0;
  int     master_node_size = 0;
  string  master_node      = "";

  it               = nodes.begin();
  master           = it->second[0];
  master_node_size = it->second.size();
  master_node      = it->first;
  it++;

  for (; it != nodes.end(); it++){
    if (master_node_size < it->second.size()){
      master           = it->second[0];
      master_node_size = it->second.size();
      master_node      = it->first;
    }
  }

  //remove master rank -- it can't be a worker
  nodes[master_node].erase(nodes[master_node].begin());
  mMaster = master;

  // am I worker or not
  // In case of CUDA and availablity of GPUs 
  // Assuming there are n gpus on the node 
  // The lowest n ranks on the node will be WORKERS and all other process will return
  // and do nothing
  // In case of OMP computation, The number of cores is split among N processes such 
  // that each process gets at least 6 threads 
  // NOTE: using more than 16  threads doesb't improve forward calculation 
 
  int numCudaDevices  = HmUtil::GetCudaDevicesCount();
  int deviceOrThreads = -1;

  int        numCores = 0;

  // Decide the possible parallelism 

  if (numCudaDevices <= 0 && parallelism == "cuda"){
    HmUtil::ExitWithError("No cuda devices available ");
  }

  else if ((parallelism == "auto" && numCudaDevices > 0) ||
	   (parallelism == "cuda" && numCudaDevices > 0) ){
    parallelism = "cuda";
  }

  else if ((parallelism == "auto" && numCudaDevices < 0) || parallelism == "omp" ){
    parallelism = "omp";
  }

  else {
    HmUtil::ExitWithError("Usupported parallelism ");
  }

  // find the order of my rank in my node
  int myorder = find(nodes[host_name].begin(), nodes[host_name].end(), rank) - 
    nodes[host_name].begin();

  if (parallelism == "cuda"){
    if (myorder < numCudaDevices && myorder < nodes[host_name].size()){
      myTask = WORKER;
      deviceIdOrNumThreads = myorder;
      bool deviceOk = HmUtil::TestCudaDevice(deviceIdOrNumThreads);

      if (!deviceOk) {
	char host[200];
	gethostname(host, 200);
	cerr << "Cuda device is down: " << host << "::" << deviceIdOrNumThreads << endl;
	myTask = NOTHING;
	deviceIdOrNumThreads = -1;
      }

    }
    else {
      myTask = NOTHING;
      deviceIdOrNumThreads = -1;
    }
  }
  else { //omp
    int numRanks = nodes[host_name].size();
    numCores = omp_get_num_procs();
    if (master_node == host_name){
      numCores--;
    }
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

  if (rank == master) myTask = MASTER;

  mParallelism = parallelism;
  mDviceOrThreads = deviceIdOrNumThreads;
  mMyTask = myTask;

}

void LeadField::Compute(){

  if (mMyTask == MASTER) {
    mUseReciprocity ? MasterReciprocity() : MasterForward();
  }
  
  else if (mMyTask == NOTHING) {
    int join = 0;
    MPI_Send(&join, 1, MPI_INT, mMaster, 0, MPI_COMM_WORLD);
    return;
  }
  else if (mMyTask == WORKER) {
    int join = 1;
    MPI_Send(&join, 1, MPI_INT, mMaster, 0, MPI_COMM_WORLD);
    mUseReciprocity ? WorkerReciprocity(mParallelism, mDviceOrThreads) : WorkerForward(mParallelism, mDviceOrThreads);
  }

  // Post processing -- 
  // compute oriented LFM if oriented dipoles are specified
  // will write oriented dipoles LFM in the specified endianness 
  // and will change units if necessary

  if (mMyTask == MASTER ) {

    bool postProcess = mSwapBytes || (mUnitStandard == "psychophysiology_nam2microv");
    
    if ( !mLfmFileNameTrip.empty() && !mLfmFileNameOri.empty()){
      WriteOrientedLfm(); // will swap bytes if specified
      if (postProcess) LfmPostProcess(mLfmFileNameTrip);
    }
    else if (mLfmFileNameTrip.empty() && !mLfmFileNameOri.empty()){
      if (postProcess) LfmPostProcess(mLfmFileNameOri);
    }
    else if (!mLfmFileNameTrip.empty() && mLfmFileNameOri.empty()){
      if (postProcess) LfmPostProcess(mLfmFileNameTrip);
    }
  }
}
