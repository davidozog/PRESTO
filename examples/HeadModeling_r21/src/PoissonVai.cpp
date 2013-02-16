///////////////////////////////////////////////////////////////////
// Poisson solver implementation file  ////////////////////////////
// NeuroInformatic Center - University of Oregon   ////////////////
// adnan@cs.uoregon.edu                            ////////////////
///////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <float.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <cassert>
#include <iomanip>

#include <cmath>
#include <omp.h>

#include <algorithm>
#include <vector>
#include <set>
#include <iterator>

#include "Poisson.h"
#include "PoissonVai.h"
#include "buffer.h"

using std::ios;
using std::setw;
using std::endl;
using std::cerr;
using namespace std;

const double PI=3.1415926535897932384626433832795;

extern "C" {

  int SolveVaiCuda(int inElementsSize, int* IJNZ, float *v1, float *AFF, float *AL,
		   int device, float Tss, float *srcs, int num_srcs , float scaledTol,
		   int maxNumIterations, int checkMax, float eps, int printFlag);

  // LU decomoposition
  int sgetrf_(long int *m, long int *n, float *a, long int *lda, long int *ipiv, long int *info);

  // Compute the inverse given LU
  int sgetri_(long int *n, float *a, long int *lda, long int *ipiv, float *work, long int *lwork, long int *info);

}

PoissonVAI::PoissonVAI():
  Poisson(){
  mNumAnisotropic = 0;
  mTangentialToNormalRatio = 1;
  mRank = 0;
  mSkullNormalCond = -1;
  mAnisotropicModel = false;

}

PoissonVAI::~PoissonVAI(){
}

int PoissonVAI::SetNormalsTan(){

  if (mNormals.empty()) {
    mNumAnisotropic = 0;
    return 0;
  }

  mNumAnisotropic = mNormals.size();

  mpCosTheta.resize(mNumAnisotropic);
  mpSinTheta.resize(mNumAnisotropic);
  mpCosPhi.resize(mNumAnisotropic);
  mpSinPhi.resize(mNumAnisotropic);

  float dirCosX, dirCosY, dirCosZ;
  int   x, y, z;

  double theta, phi;

  for(int idx=0; idx<mNumAnisotropic; idx++) {

    x = mNormals[idx][0]; 
    y = mNormals[idx][1]; 
    z = mNormals[idx][2]; 
    dirCosX = mNormals[idx][3]; 
    dirCosY = mNormals[idx][4]; 
    dirCosZ = mNormals[idx][5];
    
    mAnisotrpicElemPos.push_back(Index(x, y, z));

    theta = acos(dirCosZ);
    phi   = atan2(dirCosY, dirCosX);

    if (phi < 0 ) phi = phi + 2*PI;

    mpCosTheta[idx] = dirCosZ;
    mpSinTheta[idx] = sin(theta);

    mpCosPhi[idx] = cos(phi);
    mpSinPhi[idx] = sin(phi);

  }
  return 0;
}


int PoissonVAI::SetNormalsTanSmoothBk(){

  if (mNormals.empty()) {
    mNumAnisotropic = 0;
    return 0;
  }
  mNumAnisotropic = mNormals.size();
    
  mpCosTheta.resize(mNumAnisotropic);
  mpSinTheta.resize(mNumAnisotropic);
  mpCosPhi.resize(mNumAnisotropic);
  mpSinPhi.resize(mNumAnisotropic);

  float dirCosX, dirCosY, dirCosZ;
  int   x, y, z;

  double theta, phi;

  for(int idx=0; idx<mNumAnisotropic; idx++) {

    x = mNormals[idx][0]; 
    y = mNormals[idx][1]; 
    z = mNormals[idx][2]; 
    dirCosX = mNormals[idx][3]; 
    dirCosY = mNormals[idx][4]; 
    dirCosZ = mNormals[idx][5];

    int ijk = Index(x, y, z);
    mAnisotrpicElemPos.push_back(ijk);
    mGridVoxels[ijk].dirx = dirCosX;
    mGridVoxels[ijk].diry = dirCosY;
    mGridVoxels[ijk].dirz = dirCosZ;
  
  }

  float ratio = 1;
  float re = 3;

  for(int idx=0; idx<mNumAnisotropic; idx++) {

    int ijk =  mAnisotrpicElemPos[idx];
    VaiVoxel *vox = &mGridVoxels[ijk];

    vector<float> avgnorm;
    if (vox[-1].isanis)    avgnorm.push_back(vox[-1].dirx);
    if (vox[-mNx].isanis)  avgnorm.push_back(vox[-mNx].dirx);
    if (vox[-mNN].isanis)  avgnorm.push_back(vox[-mNN].dirx);
    if (vox[1].isanis)     avgnorm.push_back(vox[1].dirx);
    if (vox[mNx].isanis)   avgnorm.push_back(vox[mNx].dirx);
    if (vox[mNN].isanis)   avgnorm.push_back(vox[mNN].dirx);
 
    float rr = re + avgnorm.size(); 
    float sum = 0;
    for (int i=0; i<avgnorm.size(); i++){
      sum += avgnorm[i];
    }
    sum += re*vox[0].dirx;
    sum  = sum/rr;
    dirCosX = sum;

    vector<float> avgnormy;
    if (vox[-1].isanis)    avgnormy.push_back(vox[-1].diry);
    if (vox[-mNx].isanis)  avgnormy.push_back(vox[-mNx].diry);
    if (vox[-mNN].isanis)  avgnormy.push_back(vox[-mNN].diry);
    if (vox[1].isanis)     avgnormy.push_back(vox[1].diry);
    if (vox[mNx].isanis)   avgnormy.push_back(vox[mNx].diry);
    if (vox[mNN].isanis)   avgnormy.push_back(vox[mNN].diry);
 
    rr = re + avgnormy.size(); 
    sum = 0;
    for (int i=0; i<avgnormy.size(); i++){
      sum += avgnormy[i];
    }
    sum += re*vox[0].diry;
    sum  = sum/rr;
    dirCosY = sum;

    vector<float> avgnormz;
    if (vox[-1].isanis)    avgnormz.push_back(vox[-1].dirz);
    if (vox[-mNx].isanis)  avgnormz.push_back(vox[-mNx].dirz);
    if (vox[-mNN].isanis)  avgnormz.push_back(vox[-mNN].dirz);
    if (vox[1].isanis)     avgnormz.push_back(vox[1].dirz);
    if (vox[mNx].isanis)   avgnormz.push_back(vox[mNx].dirz);
    if (vox[mNN].isanis)   avgnormz.push_back(vox[mNN].dirz);
 
    rr = re + avgnormz.size(); 
    sum = 0;
    for (int i=0; i<avgnormz.size(); i++){
      sum += avgnormz[i];
    }
    sum += re*vox[0].dirz;
    sum  = sum/rr;
    dirCosZ = sum;


    theta = acos(dirCosZ);
    phi   = atan2(dirCosY, dirCosX);

    if (phi < 0 ) phi = phi + 2*PI;

    mpCosTheta[idx] = dirCosZ;
    mpSinTheta[idx] = sin(theta);

    mpCosPhi[idx] = cos(phi);
    mpSinPhi[idx] = sin(phi);

  }
  return 0;
}

void PoissonVAI::PrintInfo(ostream& out){

  Poisson::PrintInfo(out);

  out << setw(25) << "Forward algorithm: " << "VAI" << endl;
  out << setw(25) << "Conv eps: "    << mEps      << endl;
  out << setw(25) << "Conv check: "  << mCheckMax << endl;

  if (mAnisotropicModel){
      out << setw(25) << "HeadModel:" << "anisotropic" << endl;
      out << setw(25) << "Tanget to normal ratio: "  << mTangentialToNormalRatio << endl;
      out << setw(25) << "Skull normal cond: "  << mSkullNormalCond << endl;
  }
  else
    out << setw(25) << "HeadModel:" << "isotropic"  << endl;


}

void PoissonVAI::SetConductivityTensor() {

  //#pragma omp parallel for

  for (int i=0; i< (int) mGridVoxels.size(); i++){
    mGridVoxels[i].Syy = mGridVoxels[i].Sxx;
    mGridVoxels[i].Szz = mGridVoxels[i].Sxx;
  }

  //#pragma omp parallel for
  
  float  m[9];

  if (mSkullNormalCond < 0){
    mSkullNormalCond = GetTissueConds("skull");
  }
  
  for(int idx=0; idx < mNumAnisotropic; idx++) {

    int ijk  = mAnisotrpicElemPos[idx];

    float xx = mSkullNormalCond*mTangentialToNormalRatio;
    float yy = mSkullNormalCond*mTangentialToNormalRatio;
    float zz = mSkullNormalCond;
    
    m[0] =   mpCosPhi[idx]*mpCosTheta[idx];
    m[1] =   mpSinPhi[idx]*mpCosTheta[idx]; 
    m[2] =   -mpSinTheta[idx]; 

    m[3] =  -mpSinPhi[idx];
    m[4] =   mpCosPhi[idx];
    m[5] =   0;   

    m[6] =  mpSinTheta[idx]*mpCosPhi[idx];  
    m[7] =  mpSinTheta[idx]*mpSinPhi[idx];  
    m[8] =  mpCosTheta[idx];

    mGridVoxels[ijk].Sxx = xx*m[0]*m[0] + yy*m[1]*m[1] + zz*m[2]*m[2]; ///
    mGridVoxels[ijk].Syy = xx*m[3]*m[3] + yy*m[4]*m[4] + zz*m[5]*m[5]; ///
    mGridVoxels[ijk].Szz = xx*m[6]*m[6] + yy*m[7]*m[7] + zz*m[8]*m[8]; //

    mGridVoxels[ijk].Sxy = xx*m[0]*m[3] + yy*m[1]*m[4] + zz*m[2]*m[5]; //
    mGridVoxels[ijk].Sxz = xx*m[0]*m[6] + yy*m[1]*m[7] + zz*m[2]*m[8]; //
    mGridVoxels[ijk].Syz = xx*m[3]*m[6] + yy*m[4]*m[7] + zz*m[5]*m[8]; //

    mGridVoxels[ijk].Syx =  mGridVoxels[ijk].Sxy;
    mGridVoxels[ijk].Szx =  mGridVoxels[ijk].Sxz;
    mGridVoxels[ijk].Szy =  mGridVoxels[ijk].Syz;

  }
}

void PoissonVAI::BlurConds() {

  //float b = 1e-6;
  float b = 1e-3;
  vector<VaiVoxel> tGridVoxels(mGridVoxels.begin(), mGridVoxels.end()); 

  //#pragma omp parallel for 
  for(int k=0; k<mNz1; k++) {
    for(int j=0; j<mNy1; j++) {
      for(int i=0; i<mNx1; i++) {
	int idx = Index(i,j,k);
	
	VaiVoxel *mvox = &mGridVoxels[idx];
	VaiVoxel *vox = &tGridVoxels[idx];

	mvox[0].Sxx = 0.5 *(vox[0].Sxx + vox[1].Sxx);
	mvox[0].Syy = 0.5 *(vox[0].Syy + vox[mNx].Syy);
	mvox[0].Szz = 0.5 *(vox[0].Szz + vox[mNN].Szz);
	
	float den = b + fabs(vox[0].Syx + vox[1].Syx);   den *= den;
	mvox[0].Syx = 2 * (vox[0].Syx + vox[1].Syx) * (fabs(vox[0].Syx) * fabs(vox[1].Syx)) /den;

	den = b + fabs(vox[0].Szx + vox[1].Szx);   den *= den;
	mvox[0].Szx = 2 * (vox[0].Szx + vox[1].Szx) * (fabs(vox[0].Szx) * fabs(vox[1].Szx)) /den;

	den = b + fabs(vox[0].Sxy + vox[mNx].Sxy); den *= den;
	mvox[0].Sxy = 2*(vox[0].Sxy + vox[mNx].Sxy)*(fabs(vox[0].Sxy)*fabs(vox[mNx].Sxy))/den;

	den = b + fabs(vox[0].Szy + vox[mNx].Szy); den *= den;
	mvox[0].Szy = 2*(vox[0].Szy + vox[mNx].Szy)*(fabs(vox[0].Szy)*fabs(vox[mNx].Szy))/den;
	
	den = b + fabs(vox[0].Sxz + vox[mNN].Sxz); den *= den;
	mvox[0].Sxz = 2*(vox[0].Sxz + vox[mNN].Sxz)*(fabs(vox[0].Sxz)*fabs(vox[mNN].Sxz))/den;

	den = b + fabs(vox[0].Syz + vox[mNN].Syz); den *= den;
	mvox[0].Syz = 2*(vox[0].Syz + vox[mNN].Syz)*(fabs(vox[0].Syz)*fabs(vox[mNN].Syz))/den;

      }
    }
  }
}

void PoissonVAI::SmoothConductivityTensor() {

  float re = 4;
  float rr = re + 6;
  vector<VaiVoxel> tGridVoxels(mGridVoxels.begin(), mGridVoxels.end());

  for(int k=1; k<mNz1; k++) {
    for(int j=1; j<mNy1; j++) {
      for(int i=1; i<mNx1; i++) {
	int idx = Index(i,j,k);
	
	VaiVoxel *mvox = &mGridVoxels[idx];
	VaiVoxel *vox = &tGridVoxels[idx];

	mvox[0].Sxx = (1/rr) * ( re * vox[0].Sxx + vox[-1].Sxx +  vox[-mNx].Sxx + vox[-mNN].Sxx 
				 + vox[1].Sxx  +  vox[mNx].Sxx  + vox[mNN].Sxx );

	mvox[0].Syy = (1/rr) * ( re * vox[0].Syy + vox[-1].Syy +  vox[-mNx].Syy + vox[-mNN].Syy 
				 + vox[1].Syy  +  vox[mNx].Syy  + vox[mNN].Syy );

	mvox[0].Szz = (1/rr) * ( re * vox[0].Szz + vox[-1].Szz +  vox[-mNx].Szz + vox[-mNN].Szz 
				 + vox[1].Szz  +  vox[mNx].Szz  + vox[mNN].Szz );


	mvox[0].Sxy = (1/rr) * ( re * vox[0].Sxy + vox[-1].Sxy +  vox[-mNx].Sxy + vox[-mNN].Sxy 
				+ vox[1].Sxy  +  vox[mNx].Sxy  + vox[mNN].Sxy );

	mvox[0].Sxz = (1/rr) * ( re * vox[0].Sxz 
				+ vox[-1].Sxz +  vox[-mNx].Sxz + vox[-mNN].Sxz 
				+ vox[1].Sxz  +  vox[mNx].Sxz  + vox[mNN].Sxz );

	mvox[0].Syx = (1/rr) * ( re * vox[0].Syx 
				+ vox[-1].Syx +  vox[-mNx].Syx + vox[-mNN].Syx 
				+ vox[1].Syx  +  vox[mNx].Syx  + vox[mNN].Syx );

	mvox[0].Szx = (1/rr) * ( re * vox[0].Szx
				+ vox[-1].Szx +  vox[-mNx].Szx + vox[-mNN].Szx 
				+ vox[1].Szx  +  vox[mNx].Szx  + vox[mNN].Szx );

	mvox[0].Syz = (1/rr) * ( re * vox[0].Syz 
				+ vox[-1].Syz +  vox[-mNx].Syz + vox[-mNN].Syz 
				+ vox[1].Syz  +  vox[mNx].Syz  + vox[mNN].Syz );

	mvox[0].Szy = (1/rr) * ( re * vox[0].Szy 
				+ vox[-1].Szy +  vox[-mNx].Szy + vox[-mNN].Szy 
				+ vox[1].Szy  +  vox[mNx].Szy  + vox[mNN].Szy );

	if (!mvox[0].isanis && *mpGrid[idx].sigmap > .000001){
	  mvox[0].Sxx = *mpGrid[idx].sigmap;
	}

      }
    }
  }
}

void  PoissonVAI::SetConductivitiesAni(std::vector<float> conds, vector<int> tissues_idx) {
  int numTissues = mTissuesNames.size();
  for (unsigned int i=0; i<tissues_idx.size(); i++){
    int tissueIdx = tissues_idx[i];
    if (tissueIdx < numTissues) mTissuesConds[tissues_idx[i]] = conds[i];
    else if (tissueIdx == numTissues) SetSkullNormalCond(conds[i]);
    else if (tissueIdx == numTissues+1) SetTangentToNormalRatio(conds[i]);
    else {
      HmUtil::ExitWithError("Unrecognized tissue index " + HmUtil::IntToString(i));
    }
  }
  mCondsChanged = true;
}


void PoissonVAI::ScaleCondTens(){

  //#pragma omp parallel for
  for(int i=0; i<mN; i++) {
    VaiVoxel *vox = &mGridVoxels[i];
  
    vox[0].Sxx /= (mHx * mHx); 
    vox[0].Syy /= (mHy * mHy); 
    vox[0].Szz /= (mHz * mHz); 

    vox[0].Sxy /= (2*mHx*mHy); 
    vox[0].Syx /= (2*mHx*mHy); 

    vox[0].Sxz /= (2*mHx*mHz);
    vox[0].Szx /= (2*mHx*mHz);

    vox[0].Szy /= (2*mHz*mHy);
    vox[0].Syz /= (2*mHz*mHy);
  }
}


void PoissonVAI::FindElements(vector<bool>& mKey, vector<int>& mIJNZ, vector<int>& mIJK){

  int elemIdx=0;
  vector<bool>    iselem(8, false);
  vector<int>     kka(8, 0), jja(8, 0), iia(8, 0);

  /// added 
  mKey.clear();
  mIJNZ.clear();
  mIJK.clear();

  for(int k=0; k<mNz1; k++) {

    int km = max(k-1, 0);
    int kp = min(k+1, mNz1-1);

    kka[0] = kka[1] = kka[2] = kka[3] = km; 
    kka[4] = kka[5] = kka[6] = kka[7] = kp; 

    int ij0 = k%2;

    for(int j=ij0; j<mNy1; j+=2) {
      int jp = min(j+1, mNy1-1);
      int jm = max(j-1, 0);
      jja[0] = jm; jja[1] = jm; jja[2] = jp; jja[3] = jp; 
      jja[4] = jm; jja[5] = jm; jja[6] = jp; jja[7] = jp; 

      for(int i=ij0; i<mNx1; i+=2) {
	int ip = min(i+1, mNx1-1);
	int im = max(i-1, 0);
	iia[0] = im; iia[1] = ip; iia[2] = ip; iia[3] = im; 
	iia[4] = ip; iia[5] = im; iia[6] = im; iia[7] = ip; 

	iselem[0] = i>0      &&    j>0      &&   k>0      &&   mDD[Index(i, j, k)];
	iselem[1] = i<mNx1   &&    j>0      &&   k>0      &&   mDD[Index(ip, j, k)];
	iselem[2] = i<mNx1   &&    j<mNy1   &&   k>0      &&   mDD[Index(ip, jp, k)];
	iselem[3] = i>0      &&    j<mNy1   &&   k>0      &&   mDD[Index(i, jp, k)];
	iselem[4] = i<mNx1   &&    j>0      &&   k<mNz1   &&   mDD[Index(ip, j, kp)];
	iselem[5] = i>0      &&    j>0      &&   k<mNz1   &&   mDD[Index(i, j, kp)];
	iselem[6] = i>0      &&    j<mNy1   &&   k<mNz1   &&   mDD[Index(i, jp, kp)];
	iselem[7] = i<mNx1   &&    j<mNy1   &&   k<mNz1   &&   mDD[Index(ip, jp, kp)];

	bool isElement = (count(iselem.begin(), iselem.end(), true) >= 1);
	int ijk = Index(i,j,k);

	mKey.push_back(isElement);
      	mIJK.push_back(ijk);

	if (isElement){
	  int nIdx;
	  for (int c=0; c<8; c++){
	    if (k % 2 == 1){
	      nIdx = mNx*mNy*(int(kka[c]+1)/2)/4+(mNx/2-1)*(mNy/2-1)*(int((kka[c])/2))
		+ mNx*int((jja[c])/2)/2 + int((iia[c]+1)/2);
	    }
	    else
	      {
		nIdx = mNx*mNy*int((kka[c]+1)/2)/4+(mNx/2-1)*(mNy/2-1)*(int((kka[c])/2)) +
		  (mNx/2-1)*int((jja[c]-1)/2)+int((iia[c])/2);
	      }
	    mIJNZ.push_back(nIdx);
	  }
	}
	else{
	  mIJNZ.insert(mIJNZ.end(), 8, elemIdx);
	}
	elemIdx++;
      }
    }
  }
}

int PoissonVAI::UpdateCurrentSources(){
 
  //  cout << "Updating current source " << endl;

  for (int i=0; i<mGridVoxels.size(); i++){
    mGridVoxels[i].source = 0.0;
  }

  map<int, float>::const_iterator it = mCurrentSources.begin();
  for (; it != mCurrentSources.end(); it++){
    mGridVoxels[it->first].source = it->second; 
  }

  vector<float> currentSource(8,0);
  mNumSources     = 0;
  mSources.clear();

  for (int i=0; i<mInElementsSize; i++){
    int ijk = mInIJK[i];
    mInIJNZ[i*8] /= 10; //note sources Index is incoded as the ones digit (remove all sources).
    mInIJNZ[i*8] *= 10;
    if (GetCurrentSourceElement(ijk, currentSource)){
	mSources.insert(mSources.end(), currentSource.begin(), currentSource.end());
	mInIJNZ[i*8] += mNumSources+1;
	mNumSources++;	      
    }
  }
  mSourcesChanged = false; 
}

bool PoissonVAI::GetCurrentSourceElement(int ijk, vector<float>& currentElement){
  
  bool isElement = ((fabs(mGridVoxels[ijk].source))        >0 || (fabs(mGridVoxels[ijk+1].source))         >0 ||
		    (fabs(mGridVoxels[ijk+1+mNx].source))  >0 || (fabs(mGridVoxels[ijk+mNx].source))       >0 ||
		    (fabs(mGridVoxels[ijk+1+mNN].source))  >0 || (fabs(mGridVoxels[ijk+mNN].source))       >0 ||
		    (fabs(mGridVoxels[ijk+mNx+mNN].source))>0 || (fabs(mGridVoxels[ijk+1+mNx+mNN].source)) >0 );
  
  if (isElement){
    currentElement[0] = (mGridVoxels[ijk].source);
    currentElement[1] = (mGridVoxels[ijk+1].source);
    currentElement[2] = (mGridVoxels[ijk+1+mNx].source);
    currentElement[3] = (mGridVoxels[ijk+mNx].source);
    currentElement[4] = (mGridVoxels[ijk+1+mNN].source);
    currentElement[5] = (mGridVoxels[ijk+mNN].source);
    currentElement[6] = (mGridVoxels[ijk+mNx+mNN].source);
    currentElement[7] = (mGridVoxels[ijk+1+mNx+mNN].source);
  }
  return isElement;
}

void PoissonVAI::PartitionElements(){

  mAA.resize(mKey.size()*64, 0);
  mInElementsSize = 0;

  for (int i=0; i<mKey.size(); i++){
    int ijk = mIJK[i];
    if (mKey[i]){
      mInElemIdxMap[i] = mInElementsSize;
      mInIJK.push_back(ijk);
      mInElements.push_back(i);
      mInElementsSize++;
    }
  }

  set<int> s;
  for(int i=0; i < mInElementsSize; i++) {
    for (int j=0; j<8; j++) s.insert(mIJNZ[mInElements[i]*8+j]);
  }
  //  vector<int> relElementsVec(s.begin(), s.end());
  relElementsVec.assign(s.begin(), s.end());

  mInAFF.resize(mInElementsSize*64, 0);
  mInAL.resize(mInElementsSize*64, 0);

  #pragma omp parallel for
  for(int i=0; i < relElementsVec.size(); i++) {
    vector<float> AA(64), AL;
    int idx1 = relElementsVec[i];
    ComputeCoeffs( AA, mIJK[idx1]);
    copy(AA.begin(), AA.end(), mAA.begin()+idx1*64);

    if (mKey[idx1]){
      InvertCoeffs(AA, AL);
      int idx2 = mInElemIdxMap[idx1];
      copy(AL.begin(), AL.end(), mInAL.begin()+idx2*64);
    }
  }

  #pragma omp parallel for
  for(int i=0; i < mInElementsSize; i++) {
    for (int j=0; j<8; j++){
      int nidx = mIJNZ[mInElements[i]*8+j];
      for (int k=0; k<8; k++){
	mInAFF[i*64+k+j*8] = mAA[nidx*64+(7-j)*8+k];
      }
    }
  }

  mInIJNZ.resize(mInElementsSize*8, mInElementsSize);

#pragma omp parallel for 
  for (int i=0; i<mInElementsSize; i++){
    int idx = mInElements[i];
    for (int j=0; j<8; j++){
      if (mKey[mIJNZ[idx*8+j]])	mInIJNZ[i*8+j] = mInElemIdxMap[mIJNZ[idx*8+j]];
    }
    mInIJNZ[i*8] *= 10;
  } 
}


void PoissonVAI::UpdateElements(){

#pragma omp parallel for
  for(int i=0; i < relElementsVec.size(); i++) {
    vector<float> AA(64), AL;
    int idx1 = relElementsVec[i];
    ComputeCoeffs( AA, mIJK[idx1]);
    copy(AA.begin(), AA.end(), mAA.begin()+idx1*64);

    if (mKey[idx1]){
      InvertCoeffs(AA, AL);
      int idx2 = mInElemIdxMap[idx1];
      copy(AL.begin(), AL.end(), mInAL.begin()+idx2*64);
    }
  }

#pragma omp parallel for
  for(int i=0; i < mInElementsSize; i++) {
    for (int j=0; j<8; j++){
      int nidx = mIJNZ[mInElements[i]*8+j];
      for (int k=0; k<8; k++){
	mInAFF[i*64+k+j*8] = mAA[nidx*64+(7-j)*8+k];
      }
    }
  }

#pragma omp parallel for 
  for (int i=0; i<mInElementsSize; i++){
    // int idx = mInElements[i];
    // for (int j=0; j<8; j++){
    //  if (mKey[mIJNZ[idx*8+j]])	mInIJNZ[i*8+j] = mInElemIdxMap[mIJNZ[idx*8+j]];
    // }
    mInIJNZ[i*8] /= 10;
    mInIJNZ[i*8] *= 10;

  } 
}


void  PoissonVAI::UpdateCondTensor(){

  #pragma omp parallel for
  for(int i=0; i < relElementsVec.size(); i++) {
    vector<float> AA(64), AL;
    int idx1 = relElementsVec[i];
    ComputeCoeffs( AA, mIJK[idx1]);
    copy(AA.begin(), AA.end(), mAA.begin()+idx1*64);

    if (mKey[idx1]){
      InvertCoeffs(AA, AL);
      int idx2 = mInElemIdxMap[idx1];
      copy(AL.begin(), AL.end(), mInAL.begin()+idx2*64);
    }
  }

  #pragma omp parallel for
  for(int i=0; i < mInElementsSize; i++) {
    for (int j=0; j<8; j++){
      int nidx = mIJNZ[mInElements[i]*8+j];
      for (int k=0; k<8; k++){
	mInAFF[i*64+k+j*8] = mAA[nidx*64+(7-j)*8+k];
      }
    }
  }

#pragma omp parallel for 
  for (int i=0; i<mInElementsSize; i++){
    mInIJNZ[i*8] *= 10;
  } 
}

void PoissonVAI::init(){
  Poisson::init();
  VaiInit();
}

void PoissonVAI::reinit(){
  Poisson::reinit();
}

void PoissonVAI::CondChangedInit(){

  for (int i=0; i<mN; i++){
    mGridVoxels[i].Sxx    = *mpGrid[i].sigmap;
    if (mGridVoxels[i].Sxx < 0.0000001) mGridVoxels[i].Sxx = 2e-9;
  }

  SetConductivityTensor();
  BlurConds();
  ScaleCondTens();
  UpdateElements();
  UpdateCurrentSources();
  mCondsChanged = false;
}

void PoissonVAI::VaiInit(){

  mTss = 30/mHx;
  mGridVoxels.resize(mN);
  mDD.resize(mN);
  
  //#pragma omp parallel for 
  for (int i=0; i<mN; i++){
    mGridVoxels[i].source = mpGrid[i].source;
    mGridVoxels[i].Sxx    = *mpGrid[i].sigmap;

    if (mGridVoxels[i].Sxx < 0.0000001) mGridVoxels[i].Sxx = 2e-9;
    mDD[i] = (mGridVoxels[i].Sxx > .00001);
  
  }

  double mStart = HmUtil::GetWallTime(); 
  //  SetNormalsTan();
  SetNormalsTanSmoothBk();
  SetConductivityTensor();
  //SmoothConductivityTensor();

  BlurConds();
  ScaleCondTens();
  FindElements(mKey, mIJNZ, mIJK);
  PartitionElements();
  mInV1.resize(mInElementsSize*8, 0);
  mCondsChanged = false;
}

vector<float> PoissonVAI::ComputeSolution(){

  vector<float> u(mN, 0);

  for (int i=0; i< mInElementsSize; i++){

    int xyz = mInIJK[i];
    int indices[] = {xyz, xyz+1, xyz+mNx+1, xyz+mNx, xyz+1+mNN, xyz+mNN, xyz+mNx+mNN, xyz+mNx+1+mNN};

    for (int j=0; j<8; j++){
      int idx = indices[j];
      u[idx] = mInV1[i*8+j];
      mpGrid[idx].PP =  u[idx];
    }
  }

  return u;

}

int PoissonVAI::SolveOmpC(int inElementsSize, int* IJNZ, float *v1, float *AFF, float *AL,
			  float *srcs,  int srcsSize, int device, float scaledTol, float eps){
  int    iterations, i;
  int    vox, isttvox;
  float  uvv;
  float  diff = FLT_MAX;
  float  loop_step_diff = FLT_MAX;

  float *ff = (float*) malloc(inElementsSize*8*sizeof(float));
  memset(ff, 0, inElementsSize*8 * sizeof(float));
  memset(v1, 0, inElementsSize*8 * sizeof(float));

  int num_threads;
#pragma omp parallel
  {
  num_threads = omp_get_num_threads();

  //#pragma omp single
  //  cout << "num threads = " << num_threads << endl;
  }

  vector<float> thread_step_diff(num_threads, -1);
  int check = 0;

  for(iterations = 0; iterations < mKmax && loop_step_diff > scaledTol && check < mCheckMax; iterations++) {
    loop_step_diff = -1;

#pragma omp parallel for private(vox, isttvox)
    for (i=0; i<inElementsSize; i++) {
      int csrc = 0;
      for (int j=0; j<8; j++){
	int nid = IJNZ[i*8 + j]; //get neighbor element Index, each has 8 neigbors
	if (j==0) {              //source Index is encoded in first digit of first neighbor id 
	  isttvox = nid/10;      //neighbor element
	  csrc = nid%10;         //source element Index
	}
	else isttvox = nid;
 
	float uvv, ss=0;

	if (isttvox == inElementsSize) uvv = v1[i*8+j]; //element has Key 0
	else {
	  uvv = (v1[i*8+j] + v1[isttvox*8+7-j])/2.0;
	  for (int k=0; k<8; k++){
	    ss += AFF[i*64 + j*8 + k] * v1[isttvox*8+k];;
	  }
	}

	if (csrc)
	  ff[i*8 +j] = mTss * uvv + srcs[(csrc-1)*8+j] - ss;
	else
	  ff[i*8 +j] = mTss * uvv + - ss;
      }
    }


#pragma omp parallel for private(vox)
    for (int i=0; i<inElementsSize; i++) {
      for (int j=0; j<8; j++){
	float ss = 0;
	for (int k=0; k<8; k++){
	  ss += AL[i*64+j*8+k] * ff[i*8+k]; 
	}

	int threadId = omp_get_thread_num();

	if (fabs(ss - v1[i*8+j]) > thread_step_diff[threadId]){
	  thread_step_diff[threadId] = fabs(ss - v1[i*8+j]);
	}

	v1[i*8+j] = ss;
      }
    }

    loop_step_diff = *max_element(thread_step_diff.begin(), thread_step_diff.end());

    if (fabs(loop_step_diff - diff) < eps ) check++;
    else {
      check = 0;
      diff = loop_step_diff;
    }

    fill(thread_step_diff.begin(), thread_step_diff.end(), -1);
    //    printf("%d %d %f %f \n", iterations, check, loop_step_diff, diff); fflush(stdout);

  }
  
  return iterations;
  
}

int PoissonVAI::Solve(string& compParallelism, int cudaDeviceOrOmpThreads)
{
  if (compParallelism.empty()) compParallelism = "omp";

  map<int, float>::iterator iter;
  float minCurrent = fabs(mCurrentSources.begin()->second);

  for (iter = mCurrentSources.begin(); iter != mCurrentSources.end(); iter++){
    if (minCurrent < fabs(iter->second))
      minCurrent = fabs(iter->second);
  }

  float  scaledTol = mTolerance * minCurrent * mHx*mHy*mHz;
  float  eps = mEps * minCurrent * mHx*mHy*mHz;
  double mStart = HmUtil::GetWallTime(); 

  if (mCondsChanged)   {
    CondChangedInit();
  }
  if (mSourcesChanged) {UpdateCurrentSources();}

  fill(mInV1.begin(), mInV1.end(), 0);

  double t0        = HmUtil::GetWallTime();

  if (compParallelism == "omp"){
    mNumberOfIterations= SolveOmpC(mInElementsSize, &mInIJNZ[0], &mInV1[0], &mInAFF[0], &mInAL[0],  
				   &mSources[0], mNumSources, cudaDeviceOrOmpThreads, scaledTol, eps);
  }
  
#ifdef CUDA_ENABLED

  else if (compParallelism == "cuda") {
    
    mNumberOfIterations = SolveVaiCuda(mInElementsSize, &mInIJNZ[0], &mInV1[0], &mInAFF[0], &mInAL[0],
				     cudaDeviceOrOmpThreads,  mTss, &mSources[0], mNumSources, scaledTol, 
				     mKmax, mCheckMax, eps, mRank);
  }

#endif 

  else{
    HmUtil::ExitWithError("Error: Unsupported parallelism " + compParallelism);
  }

  //  cout << "Solution time = " << mInElementsSize << "  " << mNumberOfIterations << "  " << getWallTime() - t0 << endl;

  ComputeSolution();
  mExecutionTime = HmUtil::GetWallTime() - t0;

  /*
  if (!mCondsChanged)
    fill(mInV1.begin(), mInV1.end(), 0);
  */

  //  mSourcesChanged = false; 
  //  mCondsChanged = false; 

  return mNumberOfIterations;
}


int   PoissonVAI::SetTangentToNormalRatio(float tm) { 
  mTangentialToNormalRatio = tm; 
  mCondsChanged = true;
}

int   PoissonVAI::SetSkullNormalCond(float skull_normal_cond){
  mSkullNormalCond = skull_normal_cond;
  mCondsChanged = true;
};


void  PoissonVAI::SetConvEps(float eps) { 
  mEps = eps; 
}

void  PoissonVAI::SetConvCheck(int convergenceCheck){
  mCheckMax = convergenceCheck;
};

void PoissonVAI::ComputeCoeffs(vector<float>& AA, int voxIdx){

  int xp =   1;
  int yp =   mNx;
  int zp =   mNx*mNy;
  int xpyp = mNx+1;
  int xpzp = mNx*mNy + 1;
  int ypzp = mNx*mNy + mNx;

  const VaiVoxel *vox = &mGridVoxels[voxIdx];

  AA[0] = vox[0].Sxx+vox[0].Syy+vox[0].Szz-vox[0].Sxy - vox[0].Syx - vox[0].Sxz - vox[0].Szx - vox[0].Syz - vox[0].Szy;
  AA[1]  = -vox[0].Sxx + vox[xp].Sxy + vox[0].Syx + vox[xp].Sxz + vox[0].Szx;
  AA[2]  = -vox[xp].Sxy - vox[yp].Syx;
  AA[3]  = -vox[0].Syy + vox[0].Sxy + vox[yp].Syx + vox[yp].Syz + vox[0].Szy;
  AA[4]  = -vox[xp].Sxz - vox[zp].Szx;
  AA[5]  = -vox[0].Szz + vox[0].Sxz + vox[zp].Szx + vox[0].Syz + vox[zp].Szy;
  AA[6]  = -vox[yp].Syz - vox[zp].Szy;
  AA[7]  =  0;

  AA[8]  = -vox[0].Sxx - vox[0].Sxy - vox[0].Syx - vox[0].Sxz - vox[0].Szx;
  AA[9]  =  vox[0].Sxx + vox[xp].Syy + vox[xp].Szz + vox[xp].Sxy + vox[0].Syx + 
    vox[xp].Sxz + vox[0].Szx - vox[xp].Syz - vox[xp].Szy;
  AA[10] = -vox[xp].Syy - vox[xp].Sxy - vox[yp].Syx + vox[xpyp].Syz + vox[xp].Szy;
  AA[11] =  vox[0].Sxy + vox[yp].Syx;
  AA[12] = -vox[xp].Szz - vox[xp].Sxz - vox[zp].Szx + vox[xp].Syz + vox[xpzp].Szy;
  AA[13] =  vox[0].Sxz + vox[zp].Szx;
  AA[14] =  0;
  AA[15] = -vox[xpyp].Syz - vox[xpzp].Szy;

  AA[16] = -vox[0].Sxy - vox[0].Syx;
  AA[17] = -vox[xp].Syy + vox[xp].Sxy + vox[0].Syx - vox[xp].Syz - vox[xp].Szy;
  AA[18] = vox[yp].Sxx + vox[xp].Syy + vox[xpyp].Szz - vox[xp].Sxy - vox[yp].Syx + 
    vox[xpyp].Sxz + vox[yp].Szx + vox[xpyp].Syz + vox[xp].Szy;
  AA[19] = -vox[yp].Sxx + vox[0].Sxy + vox[yp].Syx - vox[yp].Sxz - vox[yp].Szx;
  AA[20] = vox[xp].Syz + vox[xpzp].Szy;
  AA[21] = 0;
  AA[22] = vox[yp].Sxz + vox[ypzp].Szx;
  AA[23] = -vox[xpyp].Szz - vox[xpyp].Sxz - vox[ypzp].Szx - vox[xpyp].Syz - vox[xpzp].Szy;

  AA[24] = -vox[0].Syy - vox[0].Sxy - vox[0].Syx - vox[0].Syz - vox[0].Szy;
  AA[25] = vox[xp].Sxy + vox[0].Syx;
  AA[26] = -vox[yp].Sxx - vox[xp].Sxy - vox[yp].Syx + vox[xpyp].Sxz + vox[yp].Szx;
  AA[27] = vox[yp].Sxx + vox[0].Syy + vox[yp].Szz + vox[0].Sxy + vox[yp].Syx - vox[yp].Sxz - vox[yp].Szx + 
    vox[yp].Syz + vox[0].Szy;
  AA[28] = 0;
  AA[29] = vox[0].Syz + vox[zp].Szy;
  AA[30] = -vox[yp].Szz + vox[yp].Sxz + vox[ypzp].Szx - vox[yp].Syz - vox[zp].Szy;
  AA[31] = -vox[xpyp].Sxz - vox[ypzp].Szx;

  AA[32] = -vox[0].Sxz - vox[0].Szx;
  AA[33] = -vox[xp].Szz + vox[xp].Sxz + vox[0].Szx - vox[xp].Syz - vox[xp].Szy;
  AA[34] = vox[xpyp].Syz + vox[xp].Szy;
  AA[35] = 0;
  AA[36] = vox[zp].Sxx + vox[xpzp].Syy + vox[xp].Szz + vox[xpzp].Sxy + vox[zp].Syx - vox[xp].Sxz - 
    vox[zp].Szx + vox[xp].Syz + vox[xpzp].Szy;

  AA[37] = -vox[zp].Sxx - vox[zp].Sxy - vox[zp].Syx + vox[0].Sxz + vox[zp].Szx;
  AA[38] = vox[zp].Sxy + vox[ypzp].Syx;
  AA[39] = -vox[xpzp].Syy - vox[xpzp].Sxy - vox[ypzp].Syx - vox[xpyp].Syz - vox[xpzp].Szy;
  
  AA[40] = -vox[0].Szz - vox[0].Sxz - vox[0].Szx - vox[0].Syz - vox[0].Szy;
  AA[41] =  vox[xp].Sxz + vox[0].Szx;
  AA[42] =  0;
  AA[43] =  vox[yp].Syz + vox[0].Szy;
  AA[44] = -vox[zp].Sxx + vox[xpzp].Sxy + vox[zp].Syx - vox[xp].Sxz - vox[zp].Szx;
  AA[45] =  vox[zp].Sxx + vox[zp].Syy + vox[0].Szz - vox[zp].Sxy - vox[zp].Syx + vox[0].Sxz + 
    vox[zp].Szx + vox[0].Syz + vox[zp].Szy;
  AA[46] = -vox[zp].Syy + vox[zp].Sxy + vox[ypzp].Syx - vox[yp].Syz - vox[zp].Szy;
  AA[47] = -vox[xpzp].Sxy - vox[ypzp].Syx;

  AA[48] = -vox[0].Syz - vox[0].Szy;
  AA[49] =  0;
  AA[50] =  vox[xpyp].Sxz + vox[yp].Szx;
  AA[51] = -vox[yp].Szz - vox[yp].Sxz - vox[yp].Szx + vox[yp].Syz + vox[0].Szy;
  AA[52] =  vox[xpzp].Sxy + vox[zp].Syx;
  AA[53] = -vox[zp].Syy - vox[zp].Sxy - vox[zp].Syx + vox[0].Syz + vox[zp].Szy;
  AA[54] =  vox[ypzp].Sxx + vox[zp].Syy + vox[yp].Szz + vox[zp].Sxy + vox[ypzp].Syx + vox[yp].Sxz + 
    vox[ypzp].Szx - vox[yp].Syz - vox[zp].Szy;
  AA[55] = -vox[ypzp].Sxx - vox[xpzp].Sxy - vox[ypzp].Syx - vox[xpyp].Sxz - vox[ypzp].Szx;

  AA[56] = 0;
  AA[57] = -vox[xp].Syz - vox[xp].Szy;
  AA[58] = -vox[xpyp].Szz + vox[xpyp].Sxz + vox[yp].Szx + vox[xpyp].Syz + vox[xp].Szy;
  AA[59] = -vox[yp].Sxz - vox[yp].Szx;
  AA[60] = -vox[xpzp].Syy + vox[xpzp].Sxy + vox[zp].Syx + vox[xp].Syz + vox[xpzp].Szy;
  AA[61] = -vox[zp].Sxy - vox[zp].Syx;
  AA[62] = -vox[ypzp].Sxx + vox[zp].Sxy + vox[ypzp].Syx + vox[yp].Sxz + vox[ypzp].Szx;
  AA[63] = vox[ypzp].Sxx + vox[xpzp].Syy + vox[xpyp].Szz - vox[xpzp].Sxy - vox[ypzp].Syx - 
    vox[xpyp].Sxz - vox[ypzp].Szx - vox[xpyp].Syz - vox[xpzp].Szy;

}


inline void PoissonVAI::InvertCoeffs (const vector<float>& AA, vector<float>& AL){
  AL = AA;

  for(int row=0; row<8; row++) AL[9*row] += mTss;

  long int N     = 8;
  long int *ipiv = new long int[N+1];
  long int lwork = N*N;
  float *work    = new float[lwork];
  long int info;

  sgetrf_(&N, &N, &AL[0], &N, ipiv, &info);
  sgetri_(&N, &AL[0],&N, ipiv, work, &lwork, &info);

  delete ipiv;
  delete work;

}
