///////////////////////////////////////////////////////////////////
// Poisson solver implementation file  ////////////////////////////
// NeuroInformatic Center - University of Oregon   ////////////////
// adnan@cs.uoregon.edu                            ////////////////
///////////////////////////////////////////////////////////////////

#include "Poisson.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>
#include <sys/time.h>
#include <unistd.h>
#include "HmUtil.h"
#include <stdio.h>
#include <string.h>

using std::ios;
using std::setw;
using std::endl;
using std::cerr;

#ifdef CUDA_ENABLED

extern "C" int get_cuda_device_count();
extern "C" bool test_cuda_device();

#endif


Poisson::Poisson() {
    mNumberOfIterations = 0;
    mExecutionTime = 0.0;
    mBoneDensityMode = false;
}

Poisson::~Poisson() {  
  delete [] mpGrid;
}

void Poisson::init() {
  int i;
  for (i=0; i<mN; i++) {
    mpGrid[i].P[0] = mpGrid[i].P[1] = mpGrid[i].P[2] = mpGrid[i].PP = 0.0;
    mpGrid[i].R[0] = mpGrid[i].R[1] = mpGrid[i].R[2] = mpGrid[i].source = 0.0;
  }

  mKmax = 1000;
  mTolerance = .002;
  mTimeStep = 2.5;
  mCurrentSources.clear();
}

void Poisson::reinit() {
  for (int i=0; i<mN; i++) {
    mpGrid[i].PP = 0.0;
    mpGrid[i].P[0] = mpGrid[i].P[1] = mpGrid[i].P[2] = 0.0;
    mpGrid[i].R[0] = mpGrid[i].R[1] = mpGrid[i].R[2] = 0.0;
  }
}

///////////////////////////////////////////////////////////////////////////
///////////////// Sensors relted methods 
///////////////////////////////////////////////////////////////////////////

int Poisson::SetSensorsMap(char * file)
{
    std::ifstream Fp(file);
    if (!Fp.is_open()) {
        std::cerr << "set_sensors: cannot open input file: "<< file << std::endl;
        exit(1);
    }
    int num_elec;
    Fp >> num_elec;
    
    
    int m =0, i, j, k, elec_id, ijk;
    
    for (m=0; m<num_elec; m++) {
        Fp >> elec_id >>  i >>  j >>  k;
        ijk = Index(i,j,k);
        
        if (ijk<0 || ijk>=mN ) {
            std::cerr << "Warning: ignored bad electrode location outside the grid ==> " << elec_id << std::endl;
            continue;
            
        } else if (*mpGrid[ijk].sigmap < 0.000001) {
            std::cerr << "Warning: electrode in the air ==> " << elec_id << std::endl;
            continue;
        }
        
        mElectrodesLocations[elec_id] = ijk;
    }
    return num_elec;
}

int Poisson::GetTopSensorId(){
  int top_sensor_id = -1;
  int top_sensor_z = -1;

  int kk;
  for (map<int, int>::iterator it = mElectrodesLocations.begin(); it != mElectrodesLocations.end(); it++){
    kk = it->second / mNN;
    if ( kk > top_sensor_z) {
      top_sensor_z = kk;
      top_sensor_id = it->first;
    }
  }
  return top_sensor_id;
}


//////////////////////////////////////////////////////////////////////////
// geometry Methodes .... setting and printing the geometry             //
// The geometry is the distribution of the tissues labels on the grid   // 
// cells                                                                //
//////////////////////////////////////////////////////////////////////////

int Poisson::SetHeadModel(const HeadModel &bk){

  
    int i, tissue;

    // TODO clean this
    mHeadModel  = &bk;   

    mNx = bk.mGeometryShape[0];
    mNy = bk.mGeometryShape[1];
    mNz = bk.mGeometryShape[2];

    mNx1 = mNx-1;
    mNy1 = mNy-1;
    mNz1 = mNz-1;
    mN = mNx*mNy*mNz;
    mNN = mNy*mNx;
    mNmax = (mNx>mNy ? (mNx>mNz ? mNx : mNz) : (mNy>mNz ? mNy : mNz));
    
    mTissuesNames = bk.mTissueNames;
    mTissuesLabels = bk.mTissueLabels;
    mTissuesConds = bk.mTissueConds;
    
    float dx, dy, dz;
    dx = bk.mVoxelSizeMm[0];
    dy = bk.mVoxelSizeMm[1];
    dz = bk.mVoxelSizeMm[2];
    mHx = dx/1000.0;
    mHy = dy/1000.0;
    mHz = dz/1000.0;
    
    mpGrid = new GridPoint[mN];

    if (!mpGrid) {
        std::cerr << "set_geometry: cann't allocate memory for the grid" 
		  << std::endl;
        exit(1);
    }
    
    memset(mpGrid, 0, mN*sizeof(GridPoint));
    
    for (i=0; i<mN; i++) {
      tissue = (int) bk.mGeometry[i];
        mpGrid[i].sigmap = &mTissuesConds[tissue];
    }


    if (!bk.mSensorsMap.empty()){
      // int num_elec = bk.sens_map.size();
      int i, j, k, elec_id, ijk;
    
      for (map<int, vector<int> >::const_iterator it = bk.mSensorsMap.begin();
	   it != bk.mSensorsMap.end(); it++){
	elec_id = it->first;
	i = it->second[0];
	j = it->second[1];
	k = it->second[2];
        ijk = Index(i,j,k);
      
        if (ijk<0 || ijk>=mN ) {
	  std::cerr << "Warning: ignored bad electrode location outside the grid ==> " 
		    << elec_id << std::endl;
	  continue;
          
        } else if (*mpGrid[ijk].sigmap < 0.000001) {
	  std::cerr << "Warning: electrode in the air ==> " << elec_id << std::endl;
	  continue;
        }
        
        mElectrodesLocations[elec_id] = ijk;
      }
    }

    if (!bk.mCraniumNormals.empty()){
      mNormals = bk.mCraniumNormals;
      mAnisotropicModel = true;
    }

    return 0;
}


void Poisson::SetBoneDensityMode(bool mode) { 
  mBoneDensityMode = mode;

  if (mBoneDensityMode){

    int ii = 0;
    if (mHeadModel->mBmdMap.empty()) { // do we have bone density data
      HmUtil::ExitWithError("Poisson::SetBoneDensityMode: requesting missing bone density data ");
    }
    
    mBoneMineralDens.resize(mHeadModel->mBmdMap.size());
    mBoneMineralConds.resize(mHeadModel->mBmdMap.size(), 0);

    for (map<int, float>::const_iterator it=mHeadModel->mBmdMap.begin(); 
	 it != mHeadModel->mBmdMap.end(); it++){
      mBoneMineralDens[ii] = it->second;
      mpGrid[it->first].sigmap = &mBoneMineralConds[ii];
      ii++;
    }

    vector<string>::iterator it = find(mTissuesNames.begin(), mTissuesNames.end(), "skull");
    if (it == mTissuesNames.end()){
      HmUtil::ExitWithError("Attentuation coeffecient file provided and no skull tissue available");
    }

    mSkullIndex = it - mTissuesNames.begin();

    // float avg = 0;
    // for (int i=0; i<HounsFieldUnits.size(); i++) avg += HounsFieldUnits[i];
    // avg /= HounsFieldUnits.size();
    // mAttenCondFactor = mTissuesConds[idx]*avg;

    SetBoneMineralConds(mTissuesConds[mSkullIndex]);

  }
}

void Poisson::SetBoneMineralConds(float bmdCondFactor) {

  /*
  float maxhf = *max_element(mHounsFieldUnits.begin(), mHounsFieldUnits.end());
  float minhf = *min_element(mHounsFieldUnits.begin(),  mHounsFieldUnits.end());
  
  float mincond = .0005;
  float maxcond = .1;

  float amin = mincond * maxhf; 
  float amax = maxcond * minhf;

  cout << "amin = " << amin << endl;
  cout << "amax = " << amax << endl;
  */

  for (int i=0; i<mBoneMineralConds.size(); i++){
    mBoneMineralConds[i] = bmdCondFactor/mBoneMineralDens[i];
  }

}

int Poisson::SetGeometry(char *file)
{

    std::string filen = file;
    std::string fileext = filen.substr(filen.length() - 4, 4);
    int err;

    if (fileext == ".txt") 
      err = SetGeometryAscii(file);
    else
      err = SetGeometryBin(file);

    if (err) return err;

    init();
    return 0;
}

int Poisson::SetGeometryAscii(char *file )
{
    int i, tissue;
    std::ifstream Fp(file);
    if (!Fp) {
        std::cerr << "set_geometry: cannot open input file: " << file << std::endl;
        exit(1);
    }

    string geom_id;
    Fp >> geom_id;

    int num_tissues;
    Fp >> mNx >> mNy >> mNz >> num_tissues;

    mNx1 = mNx-1;
    mNy1 = mNy-1;
    mNz1 = mNz-1;
    mN = mNx*mNy*mNz;
    mNN = mNy*mNx;
    mNmax = (mNx>mNy ? (mNx>mNz ? mNx : mNz) : (mNy>mNz ? mNy : mNz));
    
    mTissuesNames.resize(num_tissues);
    mTissuesLabels.resize(num_tissues);
    mTissuesConds.resize(num_tissues);
    
    for (i=0; i<num_tissues; i++) {
        Fp>>mTissuesNames[i];
        HmUtil::TrimStrSpaces(mTissuesNames[i]);
    }
    
    for (i=0; i<num_tissues; i++)
        Fp>>mTissuesLabels[i];
    for (i=0; i<num_tissues; i++)
        Fp>>mTissuesConds[i];
    
    float dx, dy, dz;
    Fp >> dx >> dy >> dz;
    mHx = dx/1000.0;
    mHy = dy/1000.0;
    mHz = dz/1000.0;
    
    // allocate memory for the computational grid
    //cout << "[1] Allocating " << sizeof(GridPoint[N]) << "Bytes of memory." << endl;

    mpGrid = new GridPoint[mN];
    if (!mpGrid) {
        std::cerr << "set_geometry: cann't allocate memory for the grid" 
		  << std::endl;
        return 1;
    }
    
    memset(mpGrid, 0, mN*sizeof(GridPoint));
    
    for (i=0; i<mN; i++) {
        Fp >> tissue;
        mpGrid[i].sigmap = &mTissuesConds[tissue];
    }
    return 0;
}

int Poisson::SetGeometryBin(char *file)
{
    int i, tissue, num_tissues;
    std::ifstream geomf(file, std::ios::in | std::ios::binary);
    
    if (!geomf.is_open()) {
        std::cerr << "set_geometry: cannot open input file: " << file 
		  << std::endl;
        exit(1);
    }
    
    char* geom_id = new char[32];
    geomf.read(geom_id, 32*sizeof(char));

    geomf.read((char*) &mNx, sizeof(unsigned int));
    geomf.read((char*) &mNy, sizeof(unsigned int));
    geomf.read((char*) &mNz, sizeof(unsigned int));
    geomf.read((char*) &num_tissues, sizeof(unsigned int));

    mNx1 = mNx-1;
    mNy1 = mNy-1;
    mNz1 = mNz-1;
    mN = mNx*mNy*mNz;
    mNN = mNy*mNx;
    mNmax = (mNx>mNy ? (mNx>mNz ? mNx : mNz) : (mNy>mNz ? mNy : mNz));
    
    mTissuesLabels.resize(num_tissues);
    mTissuesConds.resize(num_tissues);
    
    char * t = new char[32];
    for (i=0; i<num_tissues; i++) {
        geomf.read((char*) t, 32*sizeof(char));
        mTissuesNames.push_back(t);
	mTissuesNames[i] = mTissuesNames[i].substr(0,31);
        HmUtil::TrimStrSpaces(mTissuesNames[i]);
    }
    delete [] t;
    
    for (i=0; i<num_tissues; i++)
        geomf.read((char*) &mTissuesLabels[i], sizeof(unsigned char));

    for (i=0; i<num_tissues; i++)
        geomf.read((char*) &mTissuesConds[i], sizeof(float));
    
    float dv[3];
    geomf.read((char*) dv, 3*sizeof(float));
    
    mHx=dv[0]/1000; mHy=dv[1]/1000; mHz=dv[2]/1000;
   
    mpGrid = new GridPoint[mN];
    if (!mpGrid) {
        std::cerr << "set_geometry: cann't allocate memory for the grid" 
		  << std::endl;
        return 1;
    }
    memset(mpGrid, 0, mN*sizeof(GridPoint));
    
    unsigned char * tissue_tags = new unsigned char[mN];
    geomf.read((char*) tissue_tags, mN*sizeof(unsigned char));
    
    for (i=0; i<mN; i++) {
        tissue = (int) tissue_tags[i];
        mpGrid[i].sigmap = &mTissuesConds[tissue];
    }
    
    delete [] tissue_tags;
    geomf.close();
    
    return 0;
}

/////////////////////////////////////////////////////////
//////// Sources related methods 
////////////////////////////////////////////////////////

int Poisson::AddCurrentSourceIndex(int idx, float current, 
				      int source_tissue){
  if (idx <0 || idx >= mN) {
    std::cerr<<"Warning: add_current_source: source location is outside " 
	     << "the volume ... add failed " << std::endl;
    return 0;
  }
    
  std::map<int, float>::iterator iter = mCurrentSources.find(idx);
    
  if (iter != mCurrentSources.end()) {
    std::cerr << "Warning: add_current_source: There is a current source "
	      << "at this location, remove this current source first ... "
	      << "add failed" << std::endl;
    return 0;
  }
    
  mCurrentSources[idx] = (current/(mHx*mHy*mHz));
    
  if (source_tissue >= 0 && source_tissue < (int) mTissuesConds.size()){
    mOriginalCurrentSourceTissue[idx] = mpGrid[idx].sigmap;
    mpGrid[idx].sigmap = &mTissuesConds[source_tissue];
  }
    
  mpGrid[idx].source = mCurrentSources[idx];

  mSourcesChanged = true;
  
  return 1;
}

int Poisson::AddCurrentSourceIndex(int idx, float current, 
				      string source_tissue){

  vector<string>::iterator iter = 
    find(mTissuesNames.begin(), mTissuesNames.end(), source_tissue);

  if (iter != mTissuesNames.end()){
    int tlabel = mTissuesLabels[iter-mTissuesNames.begin()];
    return AddCurrentSourceIndex(idx,current, tlabel );
  }
  else 
    return AddCurrentSourceIndex(idx,current, -1);
}


int Poisson::AddCurrentSource(int xi, int xj, int xk, float current, 
				int source_tissue){
  return AddCurrentSourceIndex(Index(xi, xj, xk), current, source_tissue);
}

int Poisson::AddCurrentSource(int xi, int xj, int xk, float current, 
				string source_tissue){
  return AddCurrentSourceIndex(Index(xi, xj, xk), current, source_tissue);
}

int Poisson::AddCurrentSource(int *xx, float current, int source_tissue)
{
  return AddCurrentSource(xx[0], xx[1], xx[2], current, source_tissue);
}

int Poisson::AddCurrentSource(int *xx, float current, string source_tissue)
{
    return AddCurrentSource(xx[0], xx[1], xx[2], current, source_tissue);
}

int Poisson::AddCurrentDipole(vector<int> xx, float current, 
				int source_tissue) {
  int r1 = AddCurrentSource(xx[0], xx[1], xx[2], current, source_tissue);
  int r2 = AddCurrentSource(xx[3], xx[4], xx[5], -current, source_tissue);
  return (r1 && r2);
}

int Poisson::AddCurrentDipole(vector<int> xx, float current, 
				string source_tissue){
  int r1 = AddCurrentSource(xx[0], xx[1], xx[2], current, source_tissue);
  int r2 = AddCurrentSource(xx[3], xx[4], xx[5], -current, source_tissue);
  return (r1 && r2);
}

int Poisson::AddCurrentSource(int elec_id, float current,  int source_tissue)
{
  std::map<int, int>::iterator iter = mElectrodesLocations.find(elec_id);
  if (iter == mElectrodesLocations.end()) {
    std::cerr<< "Warning: add_current_source: Invalid electrode ID ==> " 
	     << elec_id << std::endl;    
    return 0;
  }
  return AddCurrentSourceIndex(iter->second, current, source_tissue);
}

int Poisson::RemoveCurrentSourceIndex(int idx)
{
    std::map<int, float>::iterator iter = mCurrentSources.find(idx);
    
    if (iter == mCurrentSources.end()) {
      std::cerr<<"Warning: there is no source at this location... " 
	       << "nothing removed " << idx << std::endl;
        return 0;
    }
    
    mCurrentSources.erase(iter);
    mpGrid[idx].source = 0.0;
    
    //restor orig tissue
    std::map<int, float*>::iterator titer = mOriginalCurrentSourceTissue.find(idx);
    if (titer != mOriginalCurrentSourceTissue.end()){
      mpGrid[idx].sigmap = mOriginalCurrentSourceTissue[idx];
      mOriginalCurrentSourceTissue.erase(titer);
    }
    
    // mpGrid[idx].source = mCurrentSources[idx];

    mSourcesChanged = true;
    return 1;
}

int Poisson::RemoveCurrentSource(int xi, int xj, int xk)
{
    return RemoveCurrentSourceIndex(Index(xi, xj, xk));
}

int Poisson::RemoveCurrentSource(int * xx)
{
    return RemoveCurrentSource(xx[0], xx[1], xx[2]);
}

int Poisson::RemoveCurrentSource(int elec_id)
{
    std::map<int, int>::iterator iter = mElectrodesLocations.find(elec_id);
    
    if (iter == mElectrodesLocations.end()) {
      std::cerr<<"Warning: there is no such an electrode... nothing removed " 
	       << elec_id << std::endl;
        return 0;
    }
    return RemoveCurrentSourceIndex(iter->second);
}

int Poisson::RemoveCurrentSource(std::string kind)
{
    std::map<int, float>::iterator iter;
    
    if (kind == "all")
        for (iter = mCurrentSources.begin(); iter != mCurrentSources.end(); iter++)
            RemoveCurrentSourceIndex(iter->first);
    
    else if (kind == "source")
        for (iter = mCurrentSources.begin(); iter != mCurrentSources.end(); iter++)
            if (iter->second > 0)
                RemoveCurrentSourceIndex(iter->first);
    
    else if (kind == "sink")
        for (iter = mCurrentSources.begin(); iter != mCurrentSources.end(); iter++)
            if (iter->second < 0)
                RemoveCurrentSourceIndex(iter->first);

    return 1;
}

void Poisson::UpdateCurrentValues(float current)
{
    std::map<int, float>::iterator iter;
    
    float current_val = current/(mHx*mHy*mHz);
    for (iter = mCurrentSources.begin(); iter != mCurrentSources.end(); iter++) {
        if (iter->second > 0) {
            iter->second = current_val;
            mpGrid[iter->first].source = current_val;
        }
        else {
            iter->second = -current_val;
            mpGrid[iter->first].source = -current_val;
        }
    }
}

std::vector<int> Poisson::GetCurrentSources(){
  std::vector<int> srcs;
  std::map<int, float>::iterator iter;
  
  for (iter = mCurrentSources.begin(); iter != mCurrentSources.end(); iter++) 
    srcs.push_back(iter->first);

  return srcs;
}
////////////////////////////////////////////////////////
/////////   Solution related methods //////////////////
///////////////////////////////////////////////////////

std::map<int, float> Poisson::SensorsPotentialMap()
{
    // This method return an array of the potential at the electrodes 
    // note: the index of the array is the electrode ID 
    // for example sol[2] is the potential at electrode number 2
    std::map<int, float> elec_pot;
    std::map<int, int>::iterator iter;
    
    for (iter=mElectrodesLocations.begin(); iter != mElectrodesLocations.end(); iter++)
        elec_pot[iter->first] = mpGrid[iter->second].PP;
    return elec_pot;
}


float Poisson::VoxelPotential(const vector<int>& pos){
  return mpGrid[Index(pos[0], pos[1], pos[2])].PP;
}


std::vector<float> Poisson::SensorsPotentialVec()
{
  std::vector<float> elec_pot(mElectrodesLocations.size()+1);
  std::map<int, int>::iterator iter;
    
  for (iter=mElectrodesLocations.begin(); iter != mElectrodesLocations.end(); iter++){
    elec_pot[iter->first] = mpGrid[iter->second].PP;
  }

  return elec_pot;
}

vector<float> Poisson::VolumePotentials(){
  vector<float> sol(mN, 0.0);
  for (int i=0; i<mN; i++) sol[i] = mpGrid[i].PP;
  
  return sol;
}

vector<float> Poisson::VoxelElectricField(const vector<int>&  pos){
  
  vector<float> E(3, 0.0);

  E[0] =  (mpGrid[Index(pos[0]+1, pos[1], pos[2])].PP -
	   mpGrid[Index(pos[0]-1, pos[1], pos[2])].PP) / (2.0 * mHx);

  E[1] =  (mpGrid[Index(pos[0], pos[1]+1, pos[2])].PP -
	   mpGrid[Index(pos[0], pos[1]-1, pos[2])].PP) / (2.0 * mHy);

  E[2] =  (mpGrid[Index(pos[0], pos[1], pos[2]+1)].PP -
	   mpGrid[Index(pos[0], pos[1], pos[2]-1)].PP) / (2.0 * mHz);

  return E;
}

vector<float> Poisson::VoxelElectricField(int x, int y, int z){
  
  vector<float> E(3, 0.0);

  E[0] =  (mpGrid[Index(x+1, y, z)].PP -
	   mpGrid[Index(x-1, y, z)].PP) / (2.0 * mHx);

  E[1] =  (mpGrid[Index(x, y+1, z)].PP -
	   mpGrid[Index(x, y-1, z)].PP) / (2.0 * mHy);

  E[2] =  (mpGrid[Index(x, y, z+1)].PP -
	   mpGrid[Index(x, y, z-1)].PP) / (2.0 * mHz);

  return E;
}

vector<float> Poisson::VoxelCurrentField(const vector<int>& pos){
  vector<float> J = VoxelElectricField(pos);
  float cond = *mpGrid[Index(pos[0], pos[1], pos[2])].sigmap;
  J[0] *= -cond;
  J[1] *= -cond;
  J[2] *= -cond;
  return J;
}

void Poisson::WriteSolution(const string& filen, char mode)
{
    std::ofstream outf; 
    
    if (mode == 'b') {
        outf.open(&filen[0], std::ios::out | std::ios::binary);
        for (int i=0; i<mN; i++)
            outf.write((char*)&mpGrid[i].PP, sizeof(float));
    } else {
        outf.open(&filen[0]);
        for (int i=0; i<mN; i++)
            outf << mpGrid[i].PP << std::endl;
    }

    outf.close();
    return;
}

void Poisson::PrintInfo(ostream& out){

  out << setiosflags( ios::left );

  out << setw(25) << "Dimensions: " << "[" << mNx <<"," << mNy << ","<<mNz <<"]" << endl;
  out << setw(25) << "Resolution: " << "[" << mHx <<"," << mHy << ","<<mHz <<"]" << endl;

  out << setw(25) << "Number tissues: " << mTissuesNames.size() << endl;
  out << setw(25) << "Tissue names: ";
  for (unsigned int i=0; i<mTissuesNames.size(); i++)    out << setw(6) << mTissuesNames[i] << "  ";
  out << endl;

  out << setw(25) << "Tissue conds: ";
  for (unsigned int i=0; i<mTissuesConds.size(); i++)    out << setw(6) << mTissuesConds[i] << "  ";
  out << endl;

  out << setw(25) << "Tissue labels: ";
  for (unsigned int i=0; i<mTissuesLabels.size(); i++)    out << setw(6) << mTissuesLabels[i] << "  ";
  out << endl;
  
  if (mBoneDensityMode){
    out << setw(25) << "Use bone density: " << setw(25) << "True" << endl;
    out << setw(25) << "Bone Density Factor: " << setw(25) <<  mTissuesConds[mSkullIndex] << endl;
  }
  else
    out << setw(25) << "Use bone density: " << setw(25) << "False" << endl;
  
  out << setw(25) << "Max iteration: " << mKmax << endl;
  out << setw(25) << "Tol: " << mTolerance << endl;
  //  out << setw(25) << "Time step: " << mTimeStep << endl;

}

/*
int Poisson::GetCudaDevicesCount(){
  
#ifdef CUDA_ENABLED
  return get_cuda_device_count();
#else
  return 0;
#endif 
}

bool Poisson::TestCudaDevice(){
  
#ifdef CUDA_ENABLED
  //  return test_cuda_device();
  return get_cuda_device_count();
#else
  return false;
#endif 
}
*/

void Poisson::SetConductivitiesAni(std::vector<float> conds, 
				   vector<int> tissues_idx) {   
      
  for (unsigned int i=0; i<tissues_idx.size(); i++){
    int tissueIdx = tissues_idx[i];
    if (tissueIdx < mTissuesNames.size()) {
      mTissuesConds[tissueIdx] = conds[i];
      if (mBoneDensityMode && tissueIdx == mSkullIndex ) {
	SetBoneMineralConds(mTissuesConds[mSkullIndex]);
      }
    }
  }
  mCondsChanged = true;
}

vector<float> Poisson::GetTissueConds() { return mTissuesConds; }

float Poisson::GetTissueConds(const string& tname) {
  vector<string>::iterator iter = find(mTissuesNames.begin(), 
				       mTissuesNames.end(), tname);
  if (iter == mTissuesNames.end()) HmUtil::ExitWithError("No tissue names " + 
						   tname);
  return mTissuesConds[iter-mTissuesNames.begin()];
}

float Poisson::GetTissueConds(const vector<int>& loc) {
  return *mpGrid[Index(loc[0], loc[1], loc[2])].sigmap;
}
  

  
void Poisson::SetTissueConds(std::vector<float> conds) { 
  mTissuesConds = conds; 

  if (mBoneDensityMode) {
    SetBoneMineralConds(mTissuesConds[mSkullIndex]);
  }

  mCondsChanged = true;
}

void Poisson::SetTissueConds(int idx, float value) { 
  mTissuesConds[idx] = value; 
  
  if (mBoneDensityMode && idx == mSkullIndex ) {
    SetBoneMineralConds(mTissuesConds[mSkullIndex]);
  }

  mCondsChanged = true;
}

int  Poisson::SetTissueConds(string tissue, float value) {
  vector<string>::iterator iter = 
    find(mTissuesNames.begin(), mTissuesNames.end(), tissue);
  if (iter == mTissuesNames.end()) return 1;

  // mTissuesConds[iter-mTissuesNames.begin()] = value;
  // mCondsChanged = true;

  int tissueIndex = iter-mTissuesNames.begin();
  SetTissueConds(tissueIndex, value);

  return 0;
}

void Poisson::SetTissueConds(std::vector<float> conds, vector<int> tissues_idx)
{ 
  for (unsigned int i=0; i<tissues_idx.size(); i++){
    mTissuesConds[tissues_idx[i]] = conds[i];

    if (mBoneDensityMode && tissues_idx[i] == mSkullIndex ) {
      SetBoneMineralConds(mTissuesConds[mSkullIndex]);
    }

  }

  mCondsChanged = true;

}

vector<std::string>  Poisson::GetTissueNames()  { 
  return mTissuesNames; 
}

vector<int> Poisson::GetTissueLabels() { 
  return mTissuesLabels; 
} //not used
