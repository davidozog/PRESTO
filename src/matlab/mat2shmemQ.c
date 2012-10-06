/* ==========================================================================
 * mat2shmemQ.c 
 * 
 *==========================================================================*/

#include "mex.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define JQKEY 37408
#define PERMISSIONS 0660

struct my_msgbuf {
    long mtype;
    signed char mdata[2048];
};


void put_shmem_mesg(const mxArray *array_ptr, int jobid, double *size) {

  signed char *pr;
  pr = (signed char *)mxGetData(array_ptr);

  int i;

  struct my_msgbuf buf;
  int msqid;
  key_t key;

  /* connect to the queue */
  if ((msqid = msgget( JQKEY, PERMISSIONS )) == -1) { 
      perror("msgget");
      exit(1);
  }
  
  long mysize;
  mysize = (long)*size;

  memcpy(&buf.mtype, &mysize, sizeof(long));
/*  memcpy(&buf.info.job_id, &jobid, sizeof(int)); */
  memcpy(buf.mdata, pr, (int)*size*sizeof(signed char));

      /* note that pr[1] is the mesgtyp, but in this case, it's the jobid */
      if (msgsnd(msqid, &buf, sizeof(struct my_msgbuf) - sizeof(long), 0) == -1) {
          perror("msgsend");
          exit(1);
      } else {
        
        return;
      }

}


/*  MEX GATEWAY  */
void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{

  int num, i;
  mwSize idx;
  double *arg_p, *jobid, *size;

  arg_p = mxGetPr(prhs[0]);
  size = mxGetPr(prhs[1]);
  jobid = mxGetPr(prhs[2]);

  /* TODO: This uses 2x the minimal necessary memory */
 /*  UINT8_T data[(int)arg_p[1]]; */
/*
  UINT8_T data[4096];
  UINT8_T *mesg_data;
*/

  put_shmem_mesg(prhs[0], (int)*jobid, size);

/*
  mesg_data = mxCalloc((int)arg_p[0], sizeof(UINT8_T));
  for ( idx = 0; idx < (int)arg_p[0]; idx++ ) {
    mesg_data[idx] = data[idx];
  }
  plhs[0] = mxCreateNumericMatrix(0, 0, mxUINT8_CLASS, mxREAL);
  printf(" mesg_data[0] : %d \n", mesg_data[0]);
  printf(" mesg_data[end] : %d \n", mesg_data[(int)arg_p[0]-1]);
  printf(" nbytes : %d \n", (int)arg_p[0]);
*/

  /* Point mxArray to dynamicData */
/*
  mxSetData(plhs[0], mesg_data);
  mxSetM(plhs[0], (int)arg_p[0]);
  mxSetN(plhs[0], 1);
*/

  return;
}



