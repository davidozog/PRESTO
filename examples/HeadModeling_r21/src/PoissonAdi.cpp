///////////////////////////////////////////////////////////////////
// Poisson solver implementation file  ////////////////////////////
// NeuroInformatic Center - University of Oregon   ////////////////
// adnan@cs.uoregon.edu                            ////////////////
///////////////////////////////////////////////////////////////////

#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <float.h>
#include <fstream>
#include <iostream>
#include <cassert>

#include <cmath>
#include <omp.h>

#include "Poisson.h"
#include "PoissonAdi.h"
#include "buffer.h"
#include "HmUtil.h"

using std::ofstream;
using std::ios;
using std::setw;
using std::endl;
using std::cerr;

#ifdef CUDA_ENABLED

extern "C" 
int solve_poisson_equation_cuda_bmd(GridPoint *grid, int mNx, int mNy, int mNz, 
				float Hx, float mHy, float mHz, float time_step, 
				float mTolerance, int Kmax, int device, float *tissueConds, 
				int num_tissues, int *srcPos, float *srcVal, bool condChanged, int done);
extern "C" 
int solve_poisson_equation_cuda(GridPoint *grid, int mNx, int mNy, int mNz, 
				float Hx, float mHy, float mHz, float time_step, 
				float mTolerance, int Kmax, int device, float *tissueConds, 
				int num_tissues, int *srcPos, float *srcVal, int done);

#endif

PoissonADI::PoissonADI(): Poisson(){}
PoissonADI::~PoissonADI(){
  // free cuda device memory if using cuda
  // do nothing if omp mode
 
#ifdef CUDA_ENABLED
  SolveCuda(0, 0, 1);
#endif 
}

void PoissonADI::SolveX(float H1, float tau, buffer *_buff)
{
  int i, j, k;

#pragma omp for private(i, j, k)
  
  for (k=1; k< mNz1; k++) {
        
    buffer *buff = _buff + omp_get_thread_num() * mNmax;
    
    for (j=1; j< mNy1; j++) {
      
      buff[1].Al = 0;
      buff[1].Be = 0;
      
      for (i=1; i< mNx1; i++) {
	
	GridPoint* m  = &mpGrid[Index(i,j,k)];
	GridPoint* mm = &mpGrid[Index(i-1,j,k)];
	GridPoint* mp = &mpGrid[Index(i+1,j,k)];
                
	buff[i].A = H1*(*mm->sigmap + *m->sigmap);
	buff[i].B = H1*(*mp->sigmap + *m->sigmap);
                
	float F0 = m->PP*tau +(m->R[1] + m->R[2] +m->source);
	float Zn = 1./(buff[i].A+buff[i].B+ tau - buff[i].Al*buff[i].A);
	buff[i+1].Al = buff[i].B*Zn;
	buff[i+1].Be = (buff[i].A*buff[i].Be+F0 )*Zn;
      }
            
      mpGrid[Index(mNx1,j,k)].P[0] = 0;
      
      for (i=mNx1-1; i>= 0; i--) {
	GridPoint* ii = &mpGrid[Index(i,j,k)];
	GridPoint* ip = &mpGrid[Index(i+1,j,k)];
	ii->P[0] = buff[i+1].Al * ip->P[0] + buff[i+1].Be;
      }
            
      for (i=1; i< mNx1; i++) {
	GridPoint* m  = &mpGrid[Index(i,j,k)];
	GridPoint* mm = &mpGrid[Index(i-1,j,k)];
	GridPoint* mp = &mpGrid[Index(i+1,j,k)];
	m->R[0] = buff[i].A* mm->P[0] - (buff[i].A+buff[i].B)* m->P[0] + buff[i].B * mp->P[0];	    
      }
    }
  }
}

void PoissonADI::SolveY(float H2, float tau, buffer *_buff)
{
  int i, j, k;
    
#pragma omp for private(i, j, k)
    
  for (k=1; k<mNz1; k++) {
        
    buffer *buff = _buff + omp_get_thread_num() * mNmax;
        
    for (j=1; j<mNx1; j++) {
      
      buff[1].Al = 0;
      buff[1].Be = 0;
      
      for (i=1; i<mNy1; i++) {
	GridPoint* m  = &mpGrid[Index(j, i, k)];
	GridPoint* mp = &mpGrid[Index(j, i+1, k)];
	GridPoint* mm = &mpGrid[Index(j, i-1, k)];
        
	buff[i].A = H2*(*mm->sigmap + *m->sigmap);
	buff[i].B = H2*(*mp->sigmap + *m->sigmap);
        
	float F0 = m->PP*tau + (m->R[0] + m->R[2] +m->source);
	float Zn = 1./(buff[i].A+buff[i].B + tau -buff[i].Al*buff[i].A);
                
	buff[i+1].Al = buff[i].B*Zn;
	buff[i+1].Be = (buff[i].A*buff[i].Be+F0)*Zn;
      }
            
      mpGrid[Index(j,mNy1, k)].P[1] = 0;
            
      for (i=mNy1-1; i >= 0; i--) {
	GridPoint* ii = &mpGrid[Index(j,i,k)];
	GridPoint* ip = &mpGrid[Index(j,i+1, k)];
	ii->P[1] = buff[i+1].Al*ip->P[1] + buff[i+1].Be;
      }
            
      for (i=1; i<mNy1; i++) {
	GridPoint *m  = &mpGrid[Index(j,i,k)];
	GridPoint *mp = &mpGrid[Index(j,i+1,k)];
	GridPoint *mm = &mpGrid[Index(j,i-1,k)];
	m->R[1] = buff[i].A*mm->P[1] - (buff[i].A+buff[i].B)* m->P[1] + buff[i].B*mp->P[1];
      }
    }
  }
}

void PoissonADI::SolveZ(float H3, float tau, buffer *_buff, double *thread_step_diff)
{
  int i, j, k;
  *thread_step_diff = -1;
  
#pragma omp for private(i, j, k)
  
  for (k=1; k<mNy1;k++) {
    
    buffer *buff = _buff + omp_get_thread_num() * mNmax;
        
    for (j=1; j<mNx1;j++) {
      
      buff[1].Al = 0;
      buff[1].Be = 0;
            
      for (i=1; i<mNz1;i++) {
	GridPoint* m  = &mpGrid[Index(j,k,i)];
	GridPoint* mp = &mpGrid[Index(j,k,i+1)];
	GridPoint* mm = &mpGrid[Index(j,k,i-1)];
                
	buff[i].A = H3*( *mm->sigmap + *m->sigmap);
	buff[i].B = H3*( *mp->sigmap + *m->sigmap);
                
	float F0 = m->PP*tau +(m->R[0] +m->R[1] + m->source);
	float Zn = 1./(buff[i].A+buff[i].B+ tau - buff[i].Al*buff[i].A);
                
	buff[i+1].Al = buff[i].B*Zn;
	buff[i+1].Be = (buff[i].A*buff[i].Be+F0 )*Zn;
      }
            
      mpGrid[Index(j,k,mNz1-1)].P[2] = 0;
            
      for (i=mNz1-1; i>= 0; i--) {
	GridPoint* ii = &mpGrid[Index(j, k, i)];
	GridPoint* ip = &mpGrid[Index(j, k, i+1)];
	ii->P[2] = buff[i+1].Al*ip->P[2] + buff[i+1].Be;
        
	float PP = ii->PP;
	ii->PP = (ii->P[0] + ii->P[1] +ii->P[2] )/3.0;
                
	//time step convergence
	float loop_step_diff = fabs(PP - ii->PP);
	if (loop_step_diff > *thread_step_diff && fabs(ii->PP) > 0 )
	  *thread_step_diff = loop_step_diff;                 
      }
            
      for (i=1; i<mNz1; i++) {
	GridPoint* m =  &mpGrid[Index(j,k,i)];
	GridPoint* mm = &mpGrid[Index(j,k,i-1)];
	GridPoint* mp = &mpGrid[Index(j,k,i+1)];
	m->R[2] = (buff[i].A*mm->P[2] - (buff[i].A+buff[i].B)*m->P[2] + buff[i].B*mp->P[2] );
      }
    }
  }
}

void PoissonADI::PrintInfo(ostream& out){

  out << setiosflags( ios::left );
  out << setw(25) << "Forward algorithm: " << "ADI" << endl;
  Poisson::PrintInfo(out);
  out << setw(25) << "Time step: " << mTimeStep << endl;

}

int PoissonADI::SolveOmp(float tol, int numberThreads){

  if (numberThreads == 0) //use avail number of processors 
    numberThreads = omp_get_num_procs();
  else if (numberThreads < 0) 
    numberThreads = omp_get_max_threads();
  
  omp_set_num_threads(numberThreads);

  float   tau = mTimeStep * 12.0/(mHx+mHy+mHz);

  float   h1 = 0.5/(mHx*mHx);
  float   h2 = 0.5/(mHy*mHy);
  float   h3 = 0.5/(mHz*mHz);
    
  int    timeIteration = 0;
  double relativeStepDiff = DBL_MAX; //relative difference between successive iterations

  buffer *buff = (buffer*) malloc(sizeof(buffer) * mNmax * numberThreads);

  int num_threads = 0;
#pragma omp parallel
  {
  num_threads = omp_get_num_threads();

  //#pragma omp single
  //  cout << "num threads = " << num_threads << endl;
  }


  while (timeIteration <= mKmax && relativeStepDiff > tol) {
    //    cout << "timeIteration: " << timeIteration << endl;

    relativeStepDiff = -1;
    timeIteration = timeIteration+1;
        
#pragma omp parallel        
    {
	double threadStepDiff = -1;
      
	SolveX(h1, tau, buff);
	SolveY(h2, tau, buff);
	SolveZ(h3, tau, buff, &threadStepDiff);
        
#pragma omp critical
	{
	  if (relativeStepDiff <  threadStepDiff)
	    relativeStepDiff = threadStepDiff;
	}
    }
  }
  
  free(buff);
  return timeIteration;
}


#ifdef CUDA_ENABLED

int PoissonADI::SolveCuda(float tol, int cudaDevice, int done){

  int srcPos[2];
  float srcVal[2];
  int c = 0;

  map<int, float>::iterator iter;
  for (iter = mCurrentSources.begin(); iter != mCurrentSources.end(); iter++){
    srcPos[c] = iter->first;
    srcVal[c++] = iter->second;
  }

  if (mBoneDensityMode) {

    mNumberOfIterations = solve_poisson_equation_cuda_bmd(mpGrid, mNx, mNy, mNz, mHx, mHy, mHz, 
							  mTimeStep, tol, mKmax, cudaDevice, 
							  &mTissuesConds[0], mTissuesConds.size(), 
							  srcPos, srcVal, mCondsChanged,
							  done);
    
  }
  else {
    mNumberOfIterations = solve_poisson_equation_cuda(mpGrid, mNx, mNy, mNz, mHx, mHy, mHz, 
						      mTimeStep, tol, mKmax, cudaDevice, 
						      &mTissuesConds[0], mTissuesConds.size(), 
						      srcPos, srcVal, done);

  }
  return mNumberOfIterations;
}

#endif

int PoissonADI::Solve(string& compParallelism, int cudaDeviceOrOmpThreads)
{
  if (compParallelism.empty()) compParallelism = "omp";

  map<int, float>::iterator iter;
  float minCurrent = fabs(mCurrentSources.begin()->second);

  for (iter = mCurrentSources.begin(); iter != mCurrentSources.end(); iter++){
    if (minCurrent < fabs(iter->second))
      minCurrent = fabs(iter->second);

#ifdef DEBUG2

    int kk, jj, ii;
    int rem;

    kk = iter->first / mNN; rem = iter->first % mNN;
    jj = rem / mNx; ii = rem % mNx;

    cout << "[" << cudaDeviceOrOmpThreads << "] " << iter->first << " ["<< ii << ","
	 << jj << ", " << kk << "] " << iter->second << endl;
#endif

  }
  
  float  scaledTol = mTolerance * minCurrent * mHx*mHy*mHz;
  double t0        = HmUtil::GetWallTime();

#ifdef DEBUG2

  cout << "[" << cudaDeviceOrOmpThreads << "] " << "min_current = " << minCurrent << endl;
  cout << "[" << cudaDeviceOrOmpThreads << "] " << "scaled tol  = " << scaledTol << endl;

  for (unsigned int i = 0; i<mTissuesNames.size(); i++){
    cout << "[" << cudaDeviceOrOmpThreads << "] " << mTissuesNames[i] << "  " << mTissuesLabels[i] 
	 << "  " << mTissuesConds[i] << endl;
  }

  cout << "[" << cudaDeviceOrOmpThreads << "] " <<  "tol = " << mTolerance << endl;
  cout << "[" << cudaDeviceOrOmpThreads << "] " << "time step = " << mTimeStep << endl;  
  cout << "[" << cudaDeviceOrOmpThreads << "] " << "max iter = " << mKmax << endl;
  cout << endl;

#endif

  if (compParallelism == "omp") {
    mNumberOfIterations = SolveOmp(scaledTol, cudaDeviceOrOmpThreads);
  }

#ifdef CUDA_ENABLED

  else if (compParallelism == "cuda")
    mNumberOfIterations = SolveCuda(scaledTol, cudaDeviceOrOmpThreads, 0);

#endif 

  else{
    cerr << "Error: Unsupported parallelism " << compParallelism << endl;
    exit(1);
  }

  mExecutionTime = HmUtil::GetWallTime() - t0;
  //  cout << "Solution time = "<< mNumberOfIterations << "  " << mExecutionTime << endl;
  return mNumberOfIterations;

}

