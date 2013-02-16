#include <stdio.h>
#ifdef __DEVICE_EMULATION__
#define EMUSYNC __syncthreads()
#else
#define EMUSYNC
#endif


extern "C" int get_cuda_device_count(){

  int dev = 0;
  int deviceCount;

  cudaError_t cudaResultCode;

  cudaGetDeviceCount(&deviceCount);
  cudaResultCode =  cudaGetLastError();

  if (cudaResultCode != cudaSuccess || deviceCount == 0)
    return 0;

  else{
    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, dev);
    
    if (deviceProp.major == 9999 && deviceProp.minor == 9999)
      return 0;
    else
      return deviceCount;
  }
}


__global__ void vecAdd(int *A,int *B,int *C,int N) {
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  C[i] = A[i] + B[i];
}

extern "C" bool test_cuda_device(int deviceId){

  if( cudaSetDevice( deviceId ) != cudaSuccess) {
    fprintf(stderr, "Failed setting cuda device \n ");
    return false;
  }

  int N = 10;
  int *ah = new int[N]; 
  int *bh = new int[N];
  int *ch = new int[N];

  int *ad,*bd,*cd;

  int block_size = N;
  int num_blocks = N/block_size;

  dim3 dimBlock(block_size, 1, 1);
  dim3 dimGrid(num_blocks, 1, 1);

  for(int i=0; i<N; i++) ah[i] = bh[i] = i;

  //Allocating memory on device
  cudaMalloc((void **)&ad, N*sizeof(int));
  cudaMalloc((void **)&bd, N*sizeof(int));
  cudaMalloc((void **)&cd, N*sizeof(int));

  //copying the arrays from host to device
  cudaMemcpy(ad, ah, N*sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(bd, bh, N*sizeof(int), cudaMemcpyHostToDevice);

  //add the two vector on device
  vecAdd<<<num_blocks, block_size>>>(ad,bd,cd,N);
  cudaThreadSynchronize();

  //copying the sum back from device to host
  cudaMemcpy(ch, cd, N*sizeof(int), cudaMemcpyDeviceToHost);

  bool ok = true;
  for (int i=0; i<N; i++){
    // printf("%d :: %d = %d\n", i,  ch[i], ah[i]+bh[i]);
    if (ch[i] != ah[i]+bh[i]) {
      ok = false;
      break;
    }
  }

  cudaFree(ad);
  cudaFree(bd);
  cudaFree(cd);
  return ok;
}


unsigned int nextPow2(unsigned int x) {
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
}

template <unsigned int blockSize>
__global__ void reduce_kernel(float *d_input, float *d_output, unsigned int n) {

  extern __shared__ float sdata[];

  unsigned int tid = threadIdx.x;
  unsigned int i = blockIdx.x*(blockSize*2) + threadIdx.x;
  unsigned int gridSize = blockSize*2*gridDim.x;
  sdata[tid] = 0;

  while (i < n) {

      sdata[tid] = fmaxf(sdata[tid], d_input[i]);
      if (i + blockSize < n) 
	sdata[tid] = fmaxf(sdata[tid], d_input[i+blockSize]);
            
      i += gridSize;
    } 
  __syncthreads();

  if (blockSize >= 1024) { if (tid < 512) { sdata[tid] = fmaxf(sdata[tid], sdata[tid + 512]); } __syncthreads(); }
  if (blockSize >= 512) { if (tid < 256) { sdata[tid] = fmaxf(sdata[tid], sdata[tid + 256]); } __syncthreads(); }
  if (blockSize >= 256) { if (tid < 128) { sdata[tid] = fmaxf(sdata[tid], sdata[tid + 128]); } __syncthreads(); }
  if (blockSize >= 128) { if (tid <  64) { sdata[tid] = fmaxf(sdata[tid], sdata[tid +  64]); } __syncthreads(); }
  if (blockSize >= 1024){ if (tid < 512) { sdata[tid] = fmaxf(sdata[tid], sdata[tid + 512]); } __syncthreads(); }
    
#ifndef __DEVICE_EMULATION__
  if (tid < 32)
#endif
    {
      volatile float* smem = sdata;
      if (blockSize >=  64) { smem[tid] = fmaxf(smem[tid], smem[tid + 32]); EMUSYNC; }
      if (blockSize >=  32) { smem[tid] = fmaxf(smem[tid], smem[tid + 16]); EMUSYNC; }
      if (blockSize >=  16) { smem[tid] = fmaxf(smem[tid], smem[tid +  8]); EMUSYNC; }
      if (blockSize >=   8) { smem[tid] = fmaxf(smem[tid], smem[tid +  4]); EMUSYNC; }
      if (blockSize >=   4) { smem[tid] = fmaxf(smem[tid], smem[tid +  2]); EMUSYNC; }
      if (blockSize >=   2) { smem[tid] = fmaxf(smem[tid], smem[tid +  1]); EMUSYNC; }
    }
  if (tid == 0)
    d_output[blockIdx.x] = sdata[0];
}

void partial_reduce(float *d_input, float *d_output, unsigned int n, unsigned int threads, unsigned int blocks)
{
  switch(threads) {
  case 1024:   reduce_kernel<1024> <<<blocks, 1024, 1024*sizeof(float)>>> (d_input, d_output, n);   break;
  case 512:   reduce_kernel<512> <<<blocks, 512, 512*sizeof(float)>>> (d_input, d_output, n);   break;
  case 256:   reduce_kernel<256> <<<blocks, 256, 256*sizeof(float)>>> (d_input, d_output, n);   break;
  case 128:   reduce_kernel<128> <<<blocks, 128, 128*sizeof(float)>>> (d_input, d_output, n);   break;
  case 64:    reduce_kernel<64>  <<<blocks,  64,  64*sizeof(float)>>> (d_input, d_output, n);   break;
  case 32:    reduce_kernel<32>  <<<blocks,  32,  32*sizeof(float)>>> (d_input, d_output, n);   break;
  case 16:    reduce_kernel<16>  <<<blocks,  16,  16*sizeof(float)>>> (d_input, d_output, n);   break;
  case 8:     reduce_kernel<8>   <<<blocks,   8,   8*sizeof(float)>>> (d_input, d_output, n);   break;
  case 4:     reduce_kernel<4>   <<<blocks,   4,   4*sizeof(float)>>> (d_input, d_output, n);   break;
  case 2:     reduce_kernel<2>   <<<blocks,   2,   2*sizeof(float)>>> (d_input, d_output, n);   break;
  case 1:     reduce_kernel<1>   <<<blocks,   1,   1*sizeof(float)>>> (d_input, d_output, n);   break;
  }
}

float full_reduce(float *d_input, float *d_output, unsigned int n, unsigned int maxThreads, unsigned int maxBlocks)
{

  unsigned int threads = (n < maxThreads*2) ? nextPow2((n + 1)/ 2) : maxThreads;
  unsigned int blocks = min((n + (threads * 2 - 1)) / (threads * 2), maxBlocks);
  partial_reduce(d_input, d_output, n, threads, blocks);
  n = blocks;


  while(n > 1) {
    threads = (n < maxThreads*2) ? nextPow2((n + 1)/ 2) : maxThreads;
    blocks = min((n + (threads * 2 - 1)) / (threads * 2), maxBlocks);

    partial_reduce(d_output, d_output, n, threads, blocks);
    n = blocks;
  }
    
  float result;
  cudaMemcpy(&result, d_output, sizeof(float), cudaMemcpyDeviceToHost);
  return result;

}
