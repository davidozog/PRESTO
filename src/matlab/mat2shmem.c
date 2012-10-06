/* ==========================================================================
 * mat2shmem.c 
 * 
 *==========================================================================*/

#include "mex.h"
#include "string.h"
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h> 
#include <stdlib.h>

#define MAXCHARS 80   /* max length of string contained in each field */

char SHM_NAME[512] = "MAT2SHM";
char SEM_NAME[512] = "MATSEM";
void *pSharedMemory = NULL;
sem_t *pSemaphore = NULL;         
int fd, rc, permissions;


static void send_data_to_shmem(const mxArray *array_ptr) {
  signed char *pr;
  mwSize total_num_of_elements, index;

  permissions = (int)strtol("0600", NULL, 8);

  pr = (signed char *)mxGetData(array_ptr);
  total_num_of_elements = mxGetNumberOfElements(array_ptr);

  /*
  for (index=0; index<total_num_of_elements; index++)  {
    mexPrintf("\t");
      mexPrintf(" = %d\n", *pr++);
  }
  */

  /* Put the data into shared memory  */
  memcpy((signed char *)pSharedMemory, pr, total_num_of_elements*sizeof(signed char));
  mexPrintf("Wrote data to shmem\n");

  rc = sem_post(pSemaphore);
  if (rc) {
      mexPrintf("Releasing the semaphore failed; errno is %d\n", errno);
  }
  /* Wait for the python master process to read the data */
  sleep(2);
  if (!rc) {
    while (1 == 1){
      rc = sem_trywait(pSemaphore);
      if (rc==-1) {
        mexPrintf("Acquiring the semaphore failed; errno is %d\n", errno);
        sleep(1);
      }
      else if (rc == 0) {
        mexPrintf("Semaphore re-acquired\n");
        break;
      }
      else if (rc > 0){
        mexPrintf("Re-acquired the semaphore, but something's wrong. \n");
        break;
      }
      else {
        mexPrintf("error: unexpected return value. \n");
        exit(-1);
      }
    }
  }

}

void close_shmem_and_semaphore() {

  mexPrintf("Closing semaphore and shmem\n");

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
    mexPrintf("Unlinking the memory failed; errno is %d", errno);
  }

  /* Clean up the semaphore */
  rc = sem_close(pSemaphore);
  if (rc) {
      mexPrintf("Closing the semaphore failed; errno is %d", errno);
  }
  rc = sem_unlink(SEM_NAME);
  if (rc) {
      mexPrintf("Unlinking the semaphore failed; errno is %d", errno);
  }

  mexPrintf("worked!\n");

}

int open_shmem_and_semaphore() {

  permissions = (int)strtol("0600", NULL, 8);

  /* Open the shared memory */
  fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, permissions); 

  if (fd == -1) {
    fd = 0;
    if (errno == EACCES)
      mexPrintf("Permission to shm_unlink() the shared memory object was denied.\n");
    else if (errno == EACCES)
      mexPrintf("Permission was denied to shm_open() name in the specified mode, or O_TRUNC was specified and the caller does not have write permission on the object.\n");
    else if (errno == EEXIST) {
      mexPrintf("Both O_CREAT and O_EXCL were specified to shm_open() and the shared memory object specified by name already exists.\n");
      rc = shm_unlink(SHM_NAME);
      if (rc) {
        mexPrintf("Unlinking the memory failed; errno is %d", errno);
      }
      return -1;
    }
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
    mexPrintf("didn't work...\n");
    return -2;
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
    
    if (pSharedMemory) {
        pSemaphore = sem_open(SEM_NAME, O_CREAT, permissions, 0);
    
        if (pSemaphore == SEM_FAILED) {
          mexPrintf("Creating the semaphore failed; errno is %d", errno);
        }
        else { 
          mexPrintf("the semaphore is %p", (void *)pSemaphore);
        } 
    }
  }
  
  return 1;

}


/*  MEX GATEWAY  */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

  int i, ret;

  ret = open_shmem_and_semaphore();

  if (ret == -1) {
    mexPrintf("Shmem already exists. Trying again.\n");
    ret = open_shmem_and_semaphore();
  }
  else if (ret < 0) {
    mexPrintf("Error opening shmem/semaphore.\n");
    exit(-1);
  }


  if (ret) {
    send_data_to_shmem(prhs[0]);
  }
    
  close_shmem_and_semaphore();

  return;

}

