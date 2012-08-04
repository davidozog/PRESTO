/* ==========================================================================
 * closeshm.c 
 * 
 *==========================================================================*/

#include "mex.h"
#include "string.h"
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h> 


/*  the gateway routine.  */
void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{

  int fd, rc, permissions;
  void *pSharedMemory = NULL;
  char SHM_NAME[512] = "MAT2SHM";

  fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, permissions);

  if (fd == -1) {
    fd = 0;
    mexPrintf("Creating the shared memory failed; errno is %d\n", errno);
    mexPrintf("didn't work...\n");
  }
  else {
    /* The memory is created as a file that's 0 bytes long. Resize it. */
    rc = ftruncate(fd, 4096);
    if (rc) {
      mexPrintf("Resizing the shared memory failed; errno is %d\n", errno);
    }
    else {

      /* MMap the shared memory
         void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset); */
      pSharedMemory = mmap((void *)0, (size_t)4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      if (pSharedMemory == MAP_FAILED) {
        pSharedMemory = NULL;
        mexPrintf("MMapping the shared memory failed; errno is %d", errno);
      }
      else {
        mexPrintf("pSharedMemory = %p\n", pSharedMemory);
      }

    }
  }

  /* Un-mmap the memory... */
  rc = munmap(pSharedMemory, (size_t)4096);
  if (rc) {
    mexPrintf("Unmapping the memory failed; errno is %d", errno);
  }
  
  /* ...close the file descriptor... */
  if (-1 == close(fd)) {
    mexPrintf("Closing the memory's file descriptor failed; errno is %d", errno);
  }

  /* ...and destroy the shared memory. */
  rc = shm_unlink(SHM_NAME);
  if (rc) {
    mexPrintf("Unlinking the memory failed; errno is %d\n", errno);
  }
  else{
    mexPrintf("shmem destroyed!\n");
  }

    return;
}

