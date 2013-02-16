/*! \class PoissonADI
* \brief ADI 3D Poisson equation solver
* \author Adnan Salman
* \author NeuroInformatic Center
* \date 2012

  Each time step is split into three substeps. In each substep we solve a large number 
  of tridiagonal system of equations using  using Thomas algorithm 
  ( http://en.wikipedia.org/wiki/Tridiagonal_matrix_algorithm)

  1) First substep (x-direction computation): solve Ny*Nz independent tridiagonal system \n
  2) Second substep (y-direction computation): solve Nx*Nz tridiagonal system \n
  3) Third substep (z-direction): solve Ny*Nz tridiagonal system \n

  The amount of computation in the three directions are in the same order, however the 
  memory access pattern is different. 

  The computation must proceed in the above order and all updates are inplace in global 
  memory 


*/

#ifndef __POISSON_ADI__
#define __POISSON_ADI__

#include "grid_point.h"
#include <string>
#include <map>
#include <vector>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include "HeadModel.h"
#include "buffer.h"
#include "Poisson.h"

using std::ostream;
using std::cout;
using std::map;
using std::vector;

class PoissonADI : public Poisson {
   
  float       mTimeStep;
  int         SolveOmp(float scaledTolerance, int numThreadsOrCudaDevice);

#ifdef CUDA_ENABLED
  int         SolveCuda(float scaledTolerance, int numThreadsOrCudaDevice, int done);
#endif

  void        SolveX(float H1, float tau, buffer *_buff);
  void        SolveY(float H2, float tau, buffer *_buff);
  void        SolveZ(float H3, float tau, buffer *_buff, double *thread_step_diff);

public:

  PoissonADI();
  ~PoissonADI();
  PoissonADI(const PoissonADI& P);
    
  void        SetTimeStep(float timeStep) { mTimeStep = timeStep; }
  float       GetTimeStep() {return mTimeStep; }
  
  virtual int Solve(string& compParallelism, int cudaDeviceOrOmpThreads);
  void        PrintInfo(ostream& out = cout);

};

#endif
