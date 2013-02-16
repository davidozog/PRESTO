#include "grid_point.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <float.h>
#include <sys/time.h>

#define MAX_DIM 300
#define MAX_CONDS 20

/* 
   This a copy of solve_adi_cuda.cu that supports 
   the use of BMD as well. However 
   when not using BMD the oreginal copy is might be more effecient 
   
   meanwhile, both copies are used, but will merge them in a better way later

*/


typedef struct {
  
  GridPoint  *grid;
  int         Nx1, Ny1, Nz1;
  int         Nx, Ny, Nz, N;
  float       Hx, Hy, Hz;
  int         Kmax;
  float       time_step, tol;
  
  int         device;
  float      *PP, *Px, *Py, *Pz;
  float      *sig, *sigy;
  float      *Rx, *Ry, *Rz, *F0, *PP_diff;
  float      *hostPP;
  float      *sigmap, *sigmapy;
  float       varying_tau;
  float      *tissueConds;

  float       H1;
  float       H2;
  float       H3;

} SolverParametersBmd;


inline void checkCUDAErrorBmd(const char *msg)
{
  cudaError_t err = cudaGetLastError();
  if (cudaSuccess != err) {
    fprintf(stderr, "Cuda error: %s: %s.\n", msg, cudaGetErrorString( err) );
    exit(1);
  }
}

// Here we seperated the computation of F0 = tau * PP[idx] + Rx[idx] + Rz[idx] so 
// that threads memory access is coalesced. and then we write them in a way that 
// SolveX1 will read them coalesced as well    
__global__ void SetF0XBmd(float *F0, float *PP, float *Ry, float *Rz, float tau, 
		       int Nx, int Nyz, int src1, int src2, float srcv1, float srcv2)
{
  __shared__ float shared[16][17]; // avoid bank conflicts
    
  int x0 = blockIdx.x * blockDim.x;
  int y0 = blockIdx.y * blockDim.y;
  int idx1 = Nx  * (y0 + threadIdx.y) + x0 + threadIdx.x;
  int idx2 = Nyz * (x0 + threadIdx.y) + y0 + threadIdx.x;

  float F = tau * PP[idx1] + (Ry[idx1] + Rz[idx1]);

  if (idx1 == src1) F += srcv1;
  else if (idx1 == src2) F += srcv2;
  shared[threadIdx.y][threadIdx.x] = F;

  syncthreads();
  F0[idx2] = shared[threadIdx.x][threadIdx.y];

}


// Note that SolveX1 reads the conductivity data from a different that is writen 
// in column oreder so that memory access is coalesced

__global__ void SolveX1Bmd(float *F0, float *Px, float *sigmap, int Nx, 
			int Ny, int Nz, int Nx1, int Ny1, int Nz1, 
			float H1, float tau)
{
  int y    = blockIdx.x * blockDim.x + threadIdx.x;
  int z    = blockIdx.y * blockDim.y + threadIdx.y;    
  int idx0 = y * Nx + z*Ny*Nx;
  int idx2, idx20;
  int idx, i;
  Nx1--;
  
  // These two large (O(256)) local arrays  
  float Al[MAX_DIM];
  float Be[MAX_DIM];

  idx20 = z*Ny*Nx + y;
  float sigma0;
  float sigma1 = sigmap[idx20];
  float sigma2 = sigmap[idx20+Ny];
  
  float A, B = H1*(sigma2 + sigma1);
  float al=0, be=0;
  float Zn;
  
  int yz = z*Ny + y;
  int Nyz = Ny*Nz;
    
  // forward calculation 
  for (i=1; i<Nx1; i++) {
    Al[i] = al;
    Be[i] = be;
    
    idx = idx0 + i;
    idx2 = idx20 + i* Ny;

    sigma0 = sigma1;
    sigma1 = sigma2;
    sigma2 = sigmap[idx2+Ny];

    A = H1*(sigma0 + sigma1);
    B = H1*(sigma2 + sigma1);
        
    Zn = 1.0 / (A + B + tau - al*A);
    be = (A*be + F0[i*Nyz + yz]) * Zn;
    al = B*Zn;
  }
    
  Al[i] = al;
  Be[i] = be;

  Px[idx0 + Nx1] = 0.0;
  float px = 0.0;

  // backward calculation 
  for (i=Nx1-1; i>=0; i--) {
    idx = idx0 + i;
    px = Al[i+1] * px + Be[i+1];
    Px[idx] = px;
  }
}

__global__ void SolveX2Bmd(float *Px, float *Rx, float *sigma, int Nx, 
			int Ny, int Nz, int Nx1, int Ny1, int Nz1, 
			float H1)
{
  __shared__ float shared_Px[MAX_DIM];
  __shared__ float shared_sigma[MAX_DIM];
    
  int idx   = threadIdx.x;
  int IDX   = blockIdx.y * Nx * Ny + blockIdx.x * Nx + idx;
  //  int sidx  = blockIdx.y * Nx * Ny + blockIdx.x + threadIdx.x * Ny;

  float p1  = Px[IDX];
  float s1  = sigma[IDX];

  shared_Px[idx] = p1;
  shared_sigma[idx] = s1;
  syncthreads();

    
  if(idx > 0 && idx < Nx1-1) {
    
    float s0 = shared_sigma[idx-1];
    float s2 = shared_sigma[idx+1];
        
    float p0 = shared_Px[idx-1];
    float p2 = shared_Px[idx+1];
    
    float A = H1*(s0 + s1);
    float B = H1*(s2 + s1);
        
    Rx[IDX] = A*p0 - (A+B)*p1 + B*p2;
  }
}

////////////////////////////////////////////////////////////////////////////
/// y axis /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

// Similar to the computation in the x-direction but memory acess is coalesced
__global__ void SolveY1Bmd(float *PP, float *Py, float *Rx, float *Rz, float *sigmap, 
			int Nx, int Ny, int Nz, int Nx1, int Ny1, int Nz1, float H2, 
			float tau, int src1, int src2, float srcv1, float srcv2)
{
  int x = blockIdx.x * blockDim.x + threadIdx.x;
  int z = blockIdx.y * blockDim.y + threadIdx.y;
    
  if (x < Nx1 && z < Nz1) {
    Ny1--;
    
    int idx0 = x + z*Ny*Nx;
    int idx, i;
        
    float Al[MAX_DIM];
    float Be[MAX_DIM];
    float al=0, be=0;
    float A, B;
        
    float F0, Zn, sigma0;
    float sigma1 = sigmap[idx0];
    float sigma2 = sigmap[idx0+Nx];
        
    for (i=1; i<Ny1; i++) {
      Al[i] = al;
      Be[i] = be;
        
      idx = idx0 + i*Nx;
      
      sigma0 = sigma1;
      sigma1 = sigma2;
      sigma2 = sigmap[idx+Nx];
            
      A = H2*(sigma0 + sigma1);
      B = H2*(sigma2 + sigma1);

      F0 = tau * PP[idx] + Rx[idx] + Rz[idx];
      if (idx == src1)      F0 += srcv1;
      else if (idx == src2) F0 += srcv2;
      Zn = 1.0 / (A + B + tau - al*A);

      al = B*Zn;
      be = (A*be + F0) * Zn;
    }
        
    Al[i] = al;
    Be[i] = be;
        
    Py[idx0 + Ny1*Nx] = 0.0;
    float py = 0.0;
    
    for (i=Ny1-1; i>=0; i--) {
      idx = idx0 + i*Nx;
      py = Al[i+1] * py + Be[i+1];
      Py[idx] = py;
    }
  }
}

__global__ void SolveY2Bmd(float *Ry, float *Py, float *sigma, int Nx, 
			int Ny, int Nz, int Nx1, int Ny1, int Nz1, float H2)
{
  int idx  = (blockIdx.y+1) * Nx * Ny + (blockIdx.x + 1) * Nx + threadIdx.x;

  float s0 = sigma[idx-Nx];
  float s1 = sigma[idx];
  float s2 = sigma[idx+Nx];
    
  float A = H2*(s0 + s1);
  float B = H2*(s2 + s1);
    
  float p0 = Py[idx-Nx];
  float p1 = Py[idx];
  float p2 = Py[idx+Nx];
    
  Ry[idx] = A*p0 - (A+B)*p1 + B*p2;
}

////////////////////////////////////////////////////////////////////////////
/// z axis /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

// Similar to the computation in the y-directions with extra computation 
// of the average of potentials computed in the x-, y-, and z-directions
// and the difference of total potential from previous step for convergence 
// check at the end of the time step which require more memory access than 
// the y-direction 

__global__ void SolveZ1Bmd(float *PP, float *Px, float *Py, float *Pz, 
			float *Rx, float *Ry, 
			float *sigmap, int Nx, int Ny, int Nz,
			int Nx1, int Ny1, int Nz1, float H3, float tau, 
			float *PP_diff, int src1, int src2, float srcv1, float srcv2)
{
  int x = blockIdx.x * blockDim.x + threadIdx.x;
  int y = blockIdx.y * blockDim.y + threadIdx.y;
    
  if (x < Nx1 && y < Ny1) {
    Nz1--;
    
    int idx0 = x + y*Nx;
    int idx, i;
        
    float Al[MAX_DIM];
    float Be[MAX_DIM];
    float al=0, be=0;
    float A, B;
    
    int Nxy = Nx*Ny;
        
    float F0, Zn, sigma0;
    float sigma1 = sigmap[idx0];
    float sigma2 = sigmap[idx0+Nxy];
        
    for (i=1; i<Nz1; i++) {
      Al[i] = al;
      Be[i] = be;
        
      idx = idx0 + i*Nxy;
            
      sigma0 = sigma1;
      sigma1 = sigma2;
      sigma2 = sigmap[idx+Nxy];
            
      A = H3*(sigma0 + sigma1);
      B = H3*(sigma2 + sigma1);

      //      int sidx = (int) (idx == src1) + 2 * (int) (idx == src2);
      //      F0 = tau * PP[idx] + (Rx[idx] + Ry[idx] + dConstSources[sidx]);

      F0 = tau * PP[idx] + Rx[idx] + Ry[idx];
      if (idx == src1)      F0 += srcv1;
      else if (idx == src2) F0 += srcv2;

      Zn = 1.0 / (A + B + tau - al*A);
            
      al = B*Zn;
      be = (A*be + F0) * Zn;
    }
        
    Al[i] = al;
    Be[i] = be;

    Pz[idx0 + Nz1*Nxy] = 0.0;
    float ppz = 0.0;
    float pp, pz;
        
    for (i=Nz1-1; i>=0; i--) {
      idx = idx0 + i*Nxy;
      
      pz = Al[i+1] * ppz + Be[i+1];
      pp = (Px[idx] + Py[idx] + pz) / 3.0;
      
      // Compute the difference from the previous step 
      // for convergence check at the end of the time step
      PP_diff[idx] = abs(pp - PP[idx]);
      
      PP[idx] = pp;
      Pz[idx] = pz;
      ppz = pz;
    }
  }
}


__global__ void SolveZ2Bmd(float *Rz, float *Pz, float *sigma, int Nx, int Ny, int Nz,
			int Nx1, int Ny1, int Nz1, float H3)
{
    int Nxy = Nx*Ny;
    int idx = (blockIdx.y + 1) * Nxy + blockIdx.x * Nx + threadIdx.x;
    
    float s0 = sigma[idx-Nxy];
    float s1 = sigma[idx];
    float s2 = sigma[idx+Nxy];
    
    float A = H3*(s0 + s1);
    float B = H3*(s2 + s1);
    
    float p0 = Pz[idx-Nxy];
    float p1 = Pz[idx];
    float p2 = Pz[idx+Nxy];
    
    Rz[idx] = A*p0 - (A+B)*p1 + B*p2;
}


////////////////////////////////////////////////////////////////////////////
/// main ///////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

// reduction function declaration (implemented in reduction.cu)
float full_reduce(float *d_input, float *d_output, unsigned int n, 
		  unsigned int maxThreads, unsigned int maxBlocks);

int InitializeSolverBmd( SolverParametersBmd *arg){

  static int  setdev = 0;

  int deviceCount;
  cudaGetDeviceCount(&deviceCount);
  if (deviceCount == 0){
    fprintf(stderr, "There is no device supporting CUDA ");
    return 1;
  }

  char host[200];
  gethostname(host, 200);

  
  if (arg->device < 0 || arg->device >= deviceCount){
    fprintf(stderr, "No such cuda device = %d \n", arg->device);
    return 1;
  }

  else {
    if (!setdev){
      if( cudaSetDevice( arg->device ) != cudaSuccess) {
	fprintf(stderr, "Failed setting cuda device \n ");
	return 1;
      }
      setdev = 1;
    }
  }

  printf("[%s] using id=%d cuda device.\n", host, arg->device);

  //Make each dimension multiple of 16
  int Nx = arg->Nx1 + 15 - (arg->Nx1-1)%16;
  int Ny = arg->Ny1 + 15 - (arg->Ny1-1)%16;
  int Nz = arg->Nz1 + 15 - (arg->Nz1-1)%16;
  int N  = Nx * Ny * Nz;
    
  // read conductivity values into two flat arrays
  // one writen in row order and the other in column
  int i, j, k, idx0, idx1;
   
  // This is to hold the Potential (the solution)
  arg->hostPP = (float *)malloc(N * sizeof(float));
  
   // row order (x-direction) conductivity array 
  arg->sigmap = (float *)malloc(N * sizeof(float));

  // column order (y-direction) conductivity array 
  arg->sigmapy = (float *)malloc(N * sizeof(float));

  assert(arg->hostPP != NULL  && arg->sigmap != NULL && arg->sigmapy != NULL);
    
  memset(arg->sigmap,  0, N * sizeof(float));
  memset(arg->sigmapy, 0, N * sizeof(float));
  
  int idx2;
  for(k=0; k<arg->Nz1; k++) {
    for(j=0; j<arg->Ny1; j++) {
      for(i=0; i<arg->Nx1; i++) {
	idx0 = k*arg->Nx1*arg->Ny1 + j*arg->Nx1 + i;
	idx1 = k*Nx*Ny + j*Nx + i;
	idx2 = k*Nx*Ny + i*Ny + j;
    
	arg->sigmap[idx1]  = *arg->grid[idx0].sigmap;
	arg->sigmapy[idx2] = *arg->grid[idx0].sigmap;
      }
    }
  }
  
  arg->Nx = Nx;
  arg->Ny = Ny;
  arg->Nz = Nz;
  arg->N = N;

  // set parameters
  float tau = (arg->Hx+arg->Hy+arg->Hz)/12.0;
  float varying_tau = arg->time_step*(1.0/tau);
  arg->varying_tau = varying_tau;

  float H1 = 0.5/(arg->Hx*arg->Hx);
  float H2 = 0.5/(arg->Hy*arg->Hy);
  float H3 = 0.5/(arg->Hz*arg->Hz);

  arg->H1 = H1;
  arg->H2 = H2;
  arg->H3 = H3;

  // allocate device memory     
  assert(cudaMalloc( (void**) &arg->PP,      N*sizeof(float)) == cudaSuccess);
  assert(cudaMalloc( (void**) &arg->Px,      N*sizeof(float)) == cudaSuccess);
  assert(cudaMalloc( (void**) &arg->Py,      N*sizeof(float)) == cudaSuccess);
  assert(cudaMalloc( (void**) &arg->Pz,      N*sizeof(float)) == cudaSuccess);
  assert(cudaMalloc( (void**) &arg->Rx,      N*sizeof(float)) == cudaSuccess);
  assert(cudaMalloc( (void**) &arg->Ry,      N*sizeof(float)) == cudaSuccess);
  assert(cudaMalloc( (void**) &arg->Rz,      N*sizeof(float)) == cudaSuccess);
  assert(cudaMalloc( (void**) &arg->sig,     N*sizeof(float)) == cudaSuccess);
  assert(cudaMalloc( (void**) &arg->sigy,    N*sizeof(float)) == cudaSuccess);
  assert(cudaMalloc( (void**) &arg->F0,      N*sizeof(float)) == cudaSuccess);
  assert(cudaMalloc( (void**) &arg->PP_diff, N*sizeof(float)) == cudaSuccess);
  checkCUDAErrorBmd("memory allocation");
    
  cudaMemcpy(arg->sig, arg->sigmap, N*sizeof(float), cudaMemcpyHostToDevice);
  cudaMemcpy(arg->sigy, arg->sigmapy, N*sizeof(float), cudaMemcpyHostToDevice);
  checkCUDAErrorBmd("initialization");
  return 0; 
}

void free_memory_bmd(SolverParametersBmd *sp){

  cudaFree(sp->PP);
  cudaFree(sp->Px);
  cudaFree(sp->Py);
  cudaFree(sp->Pz);
  cudaFree(sp->Rx);
  cudaFree(sp->Ry);
  cudaFree(sp->Rz);
  cudaFree(sp->sig);
  cudaFree(sp->sigy);
  cudaFree(sp->F0);
  cudaFree(sp->PP_diff);

  free(sp->hostPP);
  free(sp->sigmap);
  free(sp->sigmapy);
  
}
  
int multipl16_array_bmd(SolverParametersBmd *sp, int pos){
  
  int z = pos/(sp->Nx1*sp->Ny1);
  int rem = pos%(sp->Nx1*sp->Ny1);
  int y = rem/sp->Nx1;
  int x = rem%sp->Nx1;

  return (z*sp->Nx*sp->Ny + y*sp->Nx + x);
}

void update_conds(SolverParametersBmd *arg ) {

  for(int k=0; k<arg->Nz1; k++) {
    for(int j=0; j<arg->Ny1; j++) {
      for(int i=0; i<arg->Nx1; i++) {
	int idx0 = k*arg->Nx1*arg->Ny1 + j*arg->Nx1 + i;
	int idx1 = k*arg->Nx*arg->Ny + j*arg->Nx + i;
	int idx2 = k*arg->Nx*arg->Ny + i*arg->Ny + j;
    
	arg->sigmap[idx1]  = *arg->grid[idx0].sigmap;
	arg->sigmapy[idx2] = *arg->grid[idx0].sigmap;
      }
    }
  }
  cudaMemcpy(arg->sig,  arg->sigmap,  arg->N*sizeof(float), cudaMemcpyHostToDevice);
  cudaMemcpy(arg->sigy, arg->sigmapy, arg->N*sizeof(float), cudaMemcpyHostToDevice);
  checkCUDAErrorBmd("UpdateCond");
}

extern "C" 
int solve_poisson_equation_cuda_bmd(GridPoint *grid, int Nx1, int Ny1, int Nz1, 
				float Hx, float Hy, float Hz, 
				float time_step, float tol, int Kmax, int device, 
				float *tissueConds, int num_tissues, int *srcPos, 
				float *srcVal, bool updateCond, int done)
{

  static SolverParametersBmd *sp = NULL;

  if (done){
    if (sp != NULL){
      free_memory_bmd(sp);
      free(sp);
      sp = NULL;
    }
    return 0;
  }


  if (sp == NULL){
    sp = (SolverParametersBmd*) malloc(sizeof(SolverParametersBmd));
    sp->grid = grid;
    sp->Nx1 = Nx1;
    sp->Ny1 = Ny1;
    sp->Nz1 = Nz1;
    sp->Hx  = Hx;
    sp->Hy  = Hy;
    sp->Hz  = Hz;
    sp->time_step = time_step;
    sp->tol = tol;
    sp->Kmax = Kmax;
    sp->device = device;
    sp->tissueConds = tissueConds;
    InitializeSolverBmd(sp);
    updateCond = false;
  }

  if (updateCond){
    printf("Updating cond "); fflush(stdout);
    update_conds(sp);
  }

  if (num_tissues > MAX_CONDS) {
    fprintf(stderr, "Error copying tissues conds to GPU ... number of tissues exceeds MAX_CONDS \n");
    return 0;
  }

  // set block sizes
  dim3 dimBlock_F0x(16, 16);
  dim3 dimGrid_F0x(sp->Nx/dimBlock_F0x.x, sp->Ny*sp->Nz/dimBlock_F0x.y);

  dim3 dimBlock_x1(16, 16);
  dim3 dimGrid_x1(sp->Ny/dimBlock_x1.x, sp->Nz/dimBlock_x1.y);
    
  dim3 dimBlock_x2(sp->Nx1);
  dim3 dimGrid_x2(sp->Ny1, sp->Nz1);
    
  dim3 dimBlock_y1(16, 16);
  dim3 dimGrid_y1(sp->Nx/dimBlock_y1.x, sp->Nz/dimBlock_y1.y);
    
  dim3 dimBlock_y2(sp->Nx1);
  dim3 dimGrid_y2(Ny1-2, Nz1-2);

  dim3 dimBlock_z1(16, 16);
  dim3 dimGrid_z1(sp->Nx/dimBlock_z1.x, sp->Ny/dimBlock_z1.y);
    
  dim3 dimBlock_z2(sp->Nx1);
  dim3 dimGrid_z2(sp->Ny1, sp->Nz1-2);

 // copy data to device memory 
  cudaMemset(sp->PP, 0, sp->N*sizeof(float));
  cudaMemset(sp->Px, 0, sp->N*sizeof(float));
  cudaMemset(sp->Py, 0, sp->N*sizeof(float));
  cudaMemset(sp->Pz, 0, sp->N*sizeof(float));
  cudaMemset(sp->Rx, 0, sp->N*sizeof(float));
  cudaMemset(sp->Ry, 0, sp->N*sizeof(float));
  cudaMemset(sp->Rz, 0, sp->N*sizeof(float));
  cudaMemset(sp->PP_diff, 0, sp->N*sizeof(float));
  checkCUDAErrorBmd("initialization");
        
  int   iter = 1;
  float pp_diff = FLT_MAX;

  int   src1  = multipl16_array_bmd(sp, srcPos[0]);
  int   src2  = multipl16_array_bmd(sp, srcPos[1]);
  float srcv1 = srcVal[0];
  float srcv2 = srcVal[1];

  do {

    SetF0XBmd <<< dimGrid_F0x, dimBlock_F0x >>> (sp->F0, sp->PP, sp->Ry, sp->Rz, sp->varying_tau, 
					      sp->Nx, sp->Ny*sp->Nz, src1, src2, srcv1, srcv2);
    checkCUDAErrorBmd("SetF0XBmd");

    SolveX1Bmd <<< dimGrid_x1, dimBlock_x1 >>> (sp->F0, sp->Px, sp->sigy, sp->Nx, sp->Ny, sp->Nz, 
					     sp->Nx1, sp->Ny1, sp->Nz1, sp->H1, sp->varying_tau);
    checkCUDAErrorBmd("SolveX1Bmd");

    SolveX2Bmd <<< dimGrid_x2, dimBlock_x2 >>> (sp->Px, sp->Rx, sp->sig, sp->Nx, sp->Ny, sp->Nz, 
					     sp->Nx1, sp->Ny1, sp->Nz1, sp->H1);
    checkCUDAErrorBmd("SolveX2Bmd");

    SolveY1Bmd <<< dimGrid_y1, dimBlock_y1 >>> (sp->PP, sp->Py, sp->Rx, sp->Rz, sp->sig, sp->Nx, 
					     sp->Ny, sp->Nz, sp->Nx1, sp->Ny1, sp->Nz1, sp->H2, 
    					     sp->varying_tau, src1, src2, srcv1, srcv2);
    checkCUDAErrorBmd("SolveY1Bmd");

    SolveY2Bmd <<< dimGrid_y2, dimBlock_y2 >>>  (sp->Ry, sp->Py, sp->sig, sp->Nx, sp->Ny, sp->Nz, 
					      sp->Nx1, sp->Ny1, sp->Nz1, sp->H2);
    checkCUDAErrorBmd("SolveY2Bmd");

    SolveZ1Bmd <<< dimGrid_z1, dimBlock_z1 >>> (sp->PP, sp->Px, sp->Py, sp->Pz, sp->Rx, sp->Ry, 
					     sp->sig, sp->Nx, sp->Ny, sp->Nz, sp->Nx1, sp->Ny1, 
					     sp->Nz1, sp->H3, sp->varying_tau, sp->PP_diff, src1, 
					     src2, srcv1, srcv2);
    checkCUDAErrorBmd("SolveZ1Bmd");

    SolveZ2Bmd <<< dimGrid_z2, dimBlock_z2 >>> (sp->Rz, sp->Pz, sp->sig, sp->Nx, sp->Ny, sp->Nz, sp->Nx1, 
					     sp->Ny1, sp->Nz1, sp->H3);
    checkCUDAErrorBmd("SolveZ2Bmd");
        
    pp_diff = full_reduce(sp->PP_diff, sp->PP_diff, sp->N, 512, 256);
    checkCUDAErrorBmd("reduce");

    cudaMemset(sp->PP_diff, 0, sp->N*sizeof(float));


  } while(++iter <= sp->Kmax && pp_diff > sp->tol);

  //  printf("termination: %d %d %f %f \n", iter, sp->Kmax, pp_diff, sp->tol);
    
  // copy data back to host and free device memory
  // assert(cudaMemcpy(sp->hostPP, sp->PP, sp->N*sizeof(float), cudaMemcpyDeviceToHost)  == cudaSuccess);

  cudaMemcpy(sp->hostPP, sp->PP, sp->N*sizeof(float), cudaMemcpyDeviceToHost);
  checkCUDAErrorBmd("copy");

  if (iter < 3) 
    fprintf(stderr, "Error computing on device %d \n", device);


  int k, j, i, idx0, idx1;

  // copy solution from flat array back into array of structs
  for(k=0; k<sp->Nz1; k++) {
    for(j=0; j<sp->Ny1; j++) {
      for(i=0; i<sp->Nx1; i++) {
	idx0 = k*sp->Nx1*sp->Ny1 + j*sp->Nx1 + i;
	idx1 = k*sp->Nx*sp->Ny + j*sp->Nx + i;
	sp->grid[idx0].PP = sp->hostPP[idx1];
	if (isnan(sp->grid[idx0].PP)){
	  fprintf(stderr, "Solve_adi_cuda: Invalid solution\n");
	  exit(1);
	}
      }
    }
  }

  //   printf("Iterative Loop time = %d %f\n ", iter, getWallTime()-t0);

  checkCUDAErrorBmd("finalization");
  return iter;
}
