/*! \class PoissonVAI
* \brief VAI 3D Poisson equation solver
* \author Adnan Salman
* \author NeuroInformatic Center
* \date 2012
*/


#ifndef __POISSON_VAI__
#define __POISSON_VAI__

#include <set>

#include "Poisson.h"
#include "voxel.h"
#include "HmUtil.h"
#include "grid_point.h"

#define USE_AOS 0

using std::ostream;
using std::cout;
using std::map;
using std::vector;
using std::set;

template <typename T>
void FreeCont(T& t ) {
    T tmp;
    t.swap( tmp );
}

class PoissonVAI : public Poisson {
   
  int               mNumAnisotropic; 
  float             mTangentialToNormalRatio;
  
  vector<float>     mpCosTheta, mpSinTheta, mpCosPhi, mpSinPhi;

  float             mTss;

  vector<VaiVoxel>  mGridVoxels;    // holds all mesh voxels 
  vector<bool>      mDD;

  vector<int>       mInElements;
  int               mInElementsSize;

  vector<int>       mAnisotrpicElemPos;

  vector<float>     mInV1; 
  map<int, int>     mInElemIdxMap;

  vector<int>       mInIJNZ;
  vector<float>     mInAFF;
  vector<float>     mInAL;
  vector<int>       mInIJK;

  vector<float>     mAA;
  vector<int>       mIJNZ, mIJK;
  vector<bool>      mKey;
  vector<int>       relElementsVec;

  vector<float>     mSources;
  int               mNumSources;
  float             mSkullNormalCond;
  float             mEps; 
  int               mCheckMax;
  int               mRank;

  int               SetNormalsTan();
  int               SetNormalsTanSmoothBk();

  void              SetConductivityTensor();
  void              SmoothConductivityTensor();
  void              BlurConds();
  void              ScaleCondTens();

  void              FindElements(vector<bool>& mKey, vector<int>& mIJNZ, vector<int>& mIJK);
  void              PartitionElements();

  void              ComputeCoeffs(vector<float>& AA, int voxIdx);
  void              InvertCoeffs (const vector<float>& AA, vector<float>& AL);

  int               UpdateCurrentSources();


  vector<float>     ComputeSolution();
  bool              IsSourceElement(int ijk);
  bool              GetCurrentSourceElement(int ijk, vector<float>& currentElement);
  void              CondChangedInit();
  void              UpdateCondTensor();
  void              UpdateElements();

  int               SolveOmpC(int inElementsSize, int* IJNZ, float *v1, float *AFF, float *AL, 
			      float *srcs, int srcsSize, int device, float scaledTol, float eps);

#ifdef CUDA_ENABLED
  int               SolveCuda(int threads_or_device);
#endif 

 public:

  PoissonVAI();
  ~PoissonVAI();
  PoissonVAI(const PoissonVAI & P);

  virtual void  init();
  virtual void  reinit();

  virtual int   SetTangentToNormalRatio(float tm);
  virtual int   SetSkullNormalCond(float skull_normal_cond);
  virtual void  SetConvEps(float eps);
  virtual void  SetConvCheck(int convergenceCheck);
  virtual void  SetConductivitiesAni(std::vector<float> conds, vector<int> tissues_idx);
  virtual int   Solve(string& compParallelism, int cudaDeviceOrOmpThreads);
  virtual void  PrintInfo(ostream& out = cout);
  virtual float GetConvEps() {return mEps;}
  virtual float GetConvCheck(){return mCheckMax;}
  void          VaiInit();
  
};

#endif
