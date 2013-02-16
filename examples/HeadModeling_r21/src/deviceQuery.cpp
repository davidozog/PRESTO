#include <stdlib.h>
#include <stdio.h>

#include <cuda.h>
#include <cuda_runtime_api.h>

int main( int argc, char** argv) {
  int deviceCount;
  cudaGetDeviceCount(&deviceCount);

  if (deviceCount == 0)
    printf("num_cuda_devices 0 \n");

  for (int dev = 0; dev < deviceCount; ++dev) {
    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, dev);

    if (dev == 0) {
      if (deviceProp.major == 9999 && deviceProp.minor == 9999)
	printf("num_cuda_devices 0 \n");
      else if (deviceCount == 1)
	printf("num_cuda_devices 1\n");
      else
	printf("num_cuda_devices %d\n", deviceCount);
    }
  }
}
