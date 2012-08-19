/* ==========================================================================
 * shmem2mat.c 
 * 
 *==========================================================================*/

#include "mex.h"
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h> 


#define MAXCHARS 80   /* max length of string contained in each field */

void get_shmem_mesg(UINT8_T *dat, int nbytes) {

  mxArray *out;

  char s[1024];
  char SHM_NAME[512] = "SHM2MAT";
  char SEM_NAME[512] = "PYSEM";
  int fd, rc, permissions, i;
  void *pSharedMemory = NULL;
  sem_t *the_semaphore = NULL;         

  permissions = (int)strtol("0600", NULL, 8);

  /* The Python worker process has already created the semaphore and shared memory. 
     I just need the handle. */
  the_semaphore = sem_open(SEM_NAME, 0);

  if (the_semaphore == SEM_FAILED) {
      the_semaphore = NULL;
      mexPrintf("Getting a handle to the semaphore failed; errno is %d\n", errno);
  }
  else {
    /* get a handle to the shared memory */
    fd = shm_open(SHM_NAME, O_RDWR, permissions);
    
    if (fd == -1) {
      mexPrintf("Couldn't get a handle to the shared memory; errno is %d", errno);
      if (errno == EACCES)
        mexPrintf("Permission to shm_unlink() the shared memory object was denied.\n");
      else if (errno == EACCES)
        mexPrintf("Permission was denied to shm_open() name in the specified mode, or O_TRUNC was specified and the caller does not have write permission on the object.\n");
      else if (errno == EEXIST)
        mexPrintf("Both O_CREAT and O_EXCL were specified to shm_open() and the shared memory object specified by name already exists.\n");
      else if (errno == EINVAL)
        mexPrintf("The name argument to shm_open() was invalid.\n");
      else if (errno == EMFILE)
        mexPrintf("The process already has the maximum number of files open.\n");
      else if (errno == ENAMETOOLONG)
        mexPrintf("The length of name exceeds PATH_MAX.\n");
      else if (errno == ENFILE)
        mexPrintf("The limit on the total number of files open on the system has been reached.\n");
      else if (errno == ENOENT)
        mexPrintf("An attempt was made to shm_open() a name that did not exist, and O_CREAT was not specified.\n");
      else if (errno == ENOENT)
        mexPrintf("An attempt was to made to shm_unlink() a name that does not exist.\n");
    }
    else {
      /* mmap it. */
      UINT8_T *mymap;
      mymap= mmap((void *)0, (size_t)4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      if (mymap == MAP_FAILED) {
        mexPrintf("MMapping the shared memory failed; errno is %d", errno);
      }
      else {
        /* mexPrintf("mymap = %p", mymap); */

        /* Wait for worker MPI-PY process to free up the semaphore. */
        rc = sem_wait(the_semaphore);
        if (rc)
          mexPrintf("shmem acquire failed: %s\n", (char *)mymap);
        else {
          for (i=0; i<nbytes; i++) {
            /*  mexPrintf("Read this from shmem: %d\n", mymap[i]); */
            dat[i] = mymap[i];
          }
          /* mexPrintf("Read %zu characters '%s'\n", strlen((char *)mymap), (char *)mymap); */
          rc = sem_post(the_semaphore);
          if (rc) {
              mexPrintf("Releasing the semaphore failed; errno is %d\n", errno);
          }
          if (!rc) {
            /* mexPrintf("worker done reading shmem\n"); */
          }
        }
      }
    }
  }

  /* Un-mmap the memory */
  rc = munmap(pSharedMemory, (size_t)4096);
  if (rc) {
      mexPrintf("Unmapping the memory failed; errno is %d\n", errno);
  }
  
  /* Close the shared memory segment's file descriptor */
  if (-1 == close(fd)) {
      mexPrintf("Closing memory's file descriptor failed; errno is %d\n", errno);
  }

  /* Close the semaphore */
  rc = sem_close(the_semaphore);
  if (rc) {
      mexPrintf("Closing the semaphore failed; errno is %d\n", errno);
  }
}





/*  the gateway routine.  */
void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{

  double *num_bytes; 
  
  num_bytes = mxGetPr(prhs[0]);

  int num;
  num = (int)*num_bytes;

  /* TODO: This uses 2x the minimal necessary memory */
  UINT8_T data[num];

  get_shmem_mesg(data, num);

  UINT8_T *mesg_data;
  mwSize idx;
  mesg_data = mxCalloc(num, sizeof(UINT8_T));
  for ( idx = 0; idx < num; idx++ ) {
    mesg_data[idx] = data[idx];
  }
  plhs[0] = mxCreateNumericMatrix(0, 0, mxUINT8_CLASS, mxREAL);

  /* Point mxArray to dynamicData */
  mxSetData(plhs[0], mesg_data);
  mxSetM(plhs[0], num);
  mxSetN(plhs[0], 1);

  return;
}



