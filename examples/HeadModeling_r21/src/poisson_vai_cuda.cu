/*

Cuda implementation of the VAI forward solver 

Optimization 2-b: Here we switched to float4 and int4 data types 
                  for more effecient memory access and computing indices.
		  Also, loop unrolling in computing the neighboring elements 
		  contribution is dones is done

Optimization 3  : Since we only use two or few current sources, here I 
                  replaced the sources array by passing the sources as parameters 
		  or copy them to constant memory. Whether there is a current source at
		  an element or not is encoded as the first digit in the first neigbing 
		  idices list
 
		  The redundant elements array is removed 
		  GPU memory usage is about 700MB

Adnan Salman: 11/1/2011

*/

#include <stdio.h>
#include <assert.h>
#include <float.h>
#include <unistd.h>


#define THREADS_PER_BLOCK 32
#define HOST_NAME_SIZE    200

// reduction function declaration (implemented in reduction.cu)
float full_reduce(float *d_input, float *d_output, unsigned int n, 
		  unsigned int maxThreads, unsigned int maxBlocks);


__device__ float DotProdVec4(float4 A, float4 B){
  return (A.x*B.x + A.y*B.y + A.z*B.z + A.w*B.w);
}

__device__ float ComputeFF(int idx, int neighborElemIdx, float4 *d4_v1, int inElementsSize, 
			   float dvv1, float4 *d4_AFF, float Tss, float du3, int j){

    float ss = 0,  uvv;

    if (neighborElemIdx != inElementsSize) {

      int    idx0  = idx*16+2*j;
      float4 ndv11 = d4_v1[neighborElemIdx*2];
      float4 ndv12 = d4_v1[neighborElemIdx*2 + 1];
      float ndv[8] = {ndv11.x, ndv11.y, ndv11.z, ndv11.w, 
		      ndv12.x, ndv12.y, ndv12.z, ndv12.w};
    
      uvv = (dvv1 + ndv[7-j])/2.0;
      ss = DotProdVec4(d4_AFF[idx0], ndv11) + DotProdVec4(d4_AFF[idx0+1], ndv12);

    }
    else uvv = dvv1;
    
    return (Tss * uvv +  du3 - ss);
}

__global__ void  DevComputeff4(int inElementsSize, int4 *d4_IJNZ, float4 *d4_srcs,
			       float4 *d4_AFF, float4 *d4_v1, float4 *ff4, float Tss){

  int idx  = blockIdx.x * blockDim.x + threadIdx.x;  // element index
  int idx0 = idx*2;                                  // type4 data index 

  if (idx <inElementsSize){
    int4 dijnz1 = d4_IJNZ[idx0];                    
    float4 dv11 = d4_v1[idx0];
    float4 du31 = make_float4(0,0,0,0);
    float4 du32 = make_float4(0,0,0,0);

    int srcid = dijnz1.x % 10;
    if (srcid){
      du31 = d4_srcs[(srcid-1)*2];
      du32 = d4_srcs[(srcid-1)*2+1];
    }

    float4 ff1;

    //make sure all argument are the same
    ff1.x = ComputeFF(idx, dijnz1.x/10, d4_v1, inElementsSize, dv11.x, d4_AFF, Tss, du31.x, 0);
    ff1.y = ComputeFF(idx, dijnz1.y, d4_v1, inElementsSize, dv11.y, d4_AFF, Tss, du31.y, 1);
    ff1.z = ComputeFF(idx, dijnz1.z, d4_v1, inElementsSize, dv11.z, d4_AFF, Tss, du31.z, 2);
    ff1.w = ComputeFF(idx, dijnz1.w, d4_v1, inElementsSize, dv11.w, d4_AFF, Tss, du31.w, 3);
    ff4[idx0] = ff1;

    dijnz1 = d4_IJNZ[idx0+1];
    dv11   = d4_v1[idx0+1];
    ff1.x = ComputeFF(idx, dijnz1.x, d4_v1, inElementsSize, dv11.x, d4_AFF, Tss, du32.x, 4);
    ff1.y = ComputeFF(idx, dijnz1.y, d4_v1, inElementsSize, dv11.y, d4_AFF, Tss, du32.y, 5);
    ff1.z = ComputeFF(idx, dijnz1.z, d4_v1, inElementsSize, dv11.z, d4_AFF, Tss, du32.z, 6);
    ff1.w = ComputeFF(idx, dijnz1.w, d4_v1, inElementsSize, dv11.w, d4_AFF, Tss, du32.w, 7);
    ff4[idx0+1] = ff1;

  }
}

__global__ void  UpdateSolution(int inElementsSize, float4 *d4_AL, float4 *d4_v1, float4 *ff4, float4 *diff4_v1){

  //TODO: use the temproray ff4 array to hold the temprary diff4_v1 data 
  int idx = blockIdx.x * blockDim.x + threadIdx.x;

  float4 dff1 = ff4[idx*2];
  float4 dff2 = ff4[idx*2+1];
  float4 dvvv;
  int idx0 = idx*16;

  float4 diffv1 = d4_v1[idx*2];

  dvvv.x = DotProdVec4(d4_AL[idx0],   dff1) + DotProdVec4(d4_AL[idx0+1], dff2);
  dvvv.y = DotProdVec4(d4_AL[idx0+2], dff1) + DotProdVec4(d4_AL[idx0+2+1], dff2);
  dvvv.z = DotProdVec4(d4_AL[idx0+4], dff1) + DotProdVec4(d4_AL[idx0+4+1], dff2);
  dvvv.w = DotProdVec4(d4_AL[idx0+6], dff1) + DotProdVec4(d4_AL[idx0+6+1], dff2);
  d4_v1[idx*2] = dvvv;

  diffv1.x = fabs(diffv1.x - dvvv.x);
  diffv1.y = fabs(diffv1.y - dvvv.y);
  diffv1.z = fabs(diffv1.z - dvvv.z);
  diffv1.w = fabs(diffv1.w - dvvv.w);
  diff4_v1[idx*2] = diffv1;

  dvvv.x = DotProdVec4(d4_AL[idx0+8],  dff1) + DotProdVec4(d4_AL[idx0+8+1], dff2);
  dvvv.y = DotProdVec4(d4_AL[idx0+10], dff1) + DotProdVec4(d4_AL[idx0+10+1], dff2);
  dvvv.z = DotProdVec4(d4_AL[idx0+12], dff1) + DotProdVec4(d4_AL[idx0+12+1], dff2);
  dvvv.w = DotProdVec4(d4_AL[idx0+14], dff1) + DotProdVec4(d4_AL[idx0+14+1], dff2);

  diffv1 = d4_v1[idx*2+1];
  d4_v1[idx*2+1] = dvvv;

  diffv1.x = fabs(diffv1.x - dvvv.x);
  diffv1.y = fabs(diffv1.y - dvvv.y);
  diffv1.z = fabs(diffv1.z - dvvv.z);
  diffv1.w = fabs(diffv1.w - dvvv.w);
  diff4_v1[idx*2+1] = diffv1;

}

int InitializeSolver( int device){

  //TODO: move all cuda initialization here 
  
  static int setdev = 0;

  int deviceCount;
  cudaGetDeviceCount(&deviceCount);
  if (deviceCount == 0){
    fprintf(stderr, "There is no device supporting CUDA ");
    return 1;
  }
  
  if (device < 0 || device >= deviceCount){
    fprintf(stderr, "No such cuda device = %d \n", device);
    return 1;
  }

  else {
    if (!setdev){
      if( cudaSetDevice( device ) != cudaSuccess) {
	fprintf(stderr, "Failed setting cuda device \n ");
	return 1;
      }
      setdev = 1;
    }
  }

  char host[HOST_NAME_SIZE];
  gethostname(host, HOST_NAME_SIZE);

  printf("[%s] using id=%d cuda device.\n", host, device);

  return 0; 

}

extern "C" int SolveVaiCuda(int inElementsSize, int* IJNZ, float *v1, float *AFF, float *AL,
			    int device, float Tss, float *srcs, int num_srcs, float scaledTol, 
			    int maxNumIterations, int checkMax, float eps, int printFlag){

  static int setdev = 0;
  if (setdev == 0){
    InitializeSolver(device);
    setdev = 1;
  }

  int   iterations, check = 0; 
  float loop_step_diff = FLT_MAX, diff = FLT_MAX;

  dim3 blocks(THREADS_PER_BLOCK);
  dim3 grid(inElementsSize/blocks.x + 1);

  float4 *d4_v1, *d4_AL, *d4_AFF, *d4_ff, *d4_srcs, *diff4_v1;
  int4   *d4_IJNZ;

  float * ff = (float *) malloc(inElementsSize*8*sizeof(float));

  assert( cudaMalloc( (void**) &d4_IJNZ, inElementsSize*2*sizeof(int4))   == cudaSuccess);
  assert( cudaMemcpy( d4_IJNZ, IJNZ, inElementsSize*8*sizeof(int), cudaMemcpyHostToDevice) == cudaSuccess);

  assert( cudaMalloc( (void**) &d4_AFF, inElementsSize*16*sizeof(float4))   == cudaSuccess);
  assert( cudaMemcpy( d4_AFF, AFF, inElementsSize*64*sizeof(float), cudaMemcpyHostToDevice) == cudaSuccess);

  assert( cudaMalloc( (void**) &d4_AL, inElementsSize*16*sizeof(float4))   == cudaSuccess);
  assert( cudaMemcpy( d4_AL, AL, inElementsSize*64*sizeof(float), cudaMemcpyHostToDevice) == cudaSuccess);

  assert( cudaMalloc( (void**) &d4_ff, inElementsSize*2*sizeof(float4))   == cudaSuccess);

  assert( cudaMalloc( (void**) &d4_srcs, num_srcs*2*sizeof(float4))   == cudaSuccess);
  assert( cudaMemcpy( d4_srcs, srcs, num_srcs*8*sizeof(float), cudaMemcpyHostToDevice) == cudaSuccess);

  assert( cudaMalloc( (void**) &d4_v1, inElementsSize*2*sizeof(float4))   == cudaSuccess);
  assert( cudaMemset( d4_v1, 0, inElementsSize*2*sizeof(float4)) == cudaSuccess);

  assert( cudaMalloc( (void**) &diff4_v1, inElementsSize*2*sizeof(float4))   == cudaSuccess);
  assert( cudaMemset( diff4_v1, -1, inElementsSize*8*sizeof(float)) == cudaSuccess);

  ///temp stuff
  assert( cudaMemset( d4_ff, 0,  inElementsSize*8*sizeof(float)) == cudaSuccess);
  
  for(iterations = 0; iterations < maxNumIterations && loop_step_diff > scaledTol && 
	check < checkMax; iterations++) {

    DevComputeff4<<<grid, blocks>>>(inElementsSize, d4_IJNZ, d4_srcs, d4_AFF, d4_v1, d4_ff, Tss);
    UpdateSolution<<<grid, blocks>>> (inElementsSize, d4_AL, d4_v1, d4_ff, diff4_v1);

    loop_step_diff = full_reduce((float*)diff4_v1, (float*) diff4_v1, inElementsSize*8, 512, 256);
    if (fabs(loop_step_diff - diff) < eps ) check++;
    else {
      check = 0;
      diff = loop_step_diff;
    }
  }

  assert(cudaMemcpy(v1, d4_v1, (inElementsSize)*2*sizeof(float4), cudaMemcpyDeviceToHost) == cudaSuccess);

  /*
  cudaError_t err = cudaMemcpy(v1, d4_v1, (inElementsSize)*2*sizeof(float4), cudaMemcpyDeviceToHost);

  if (err ==     cudaSuccess)
    printf("Success ");
  else if (err == cudaErrorInvalidValue)
    printf("cudaErrorInvalidValue ");
  else if (err == cudaErrorInvalidDevicePointer)
    printf("cudaErrorInvalidDevicePointer ");
  else if (err == cudaErrorInvalidMemcpyDirection)
    printf("cudaErrorInvalidMemcpyDirection ");
  else 
    printf("Unknown error ");
  */


  cudaThreadSynchronize();

  cudaFree(d4_IJNZ);
  cudaFree(d4_AFF);
  cudaFree(d4_AL);
  cudaFree(d4_ff);
  cudaFree(d4_srcs);
  cudaFree(diff4_v1);
  cudaFree(d4_v1);

  return iterations;
  
}
