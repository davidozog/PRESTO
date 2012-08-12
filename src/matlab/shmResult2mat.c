/* ==========================================================================
 * shmem2mat.c 
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

#define MQKEY 48963
#define PERMISSIONS 0660

struct my_msgbuf {
    long mtype;
    UINT8_T mdata[4096];
};


void get_shmem_mesg(UINT8_T *dat, const mxArray *array_ptr, int *jobid) {

  double *pr;
  pr = mxGetPr(array_ptr); 

  int i, nbytes;
  nbytes = (int)pr[0];

  struct my_msgbuf buf;
  int msqid;
  key_t key;

  if ((msqid = msgget(MQKEY, PERMISSIONS)) == -1) { /* connect to the queue */
      perror("msgget");
      exit(1);
  }
  
  for(;;) { 
      /* note that pr[1] is the mesgtyp, but in this case, it's the jobid */
      if (msgrcv(msqid, &buf, sizeof(buf.mdata), (int)pr[1], 0) == -1) {
          perror("msgrcv");
          exit(1);
      } else {
        *jobid = (int)pr[1];
        /* memcpy(&dat[0], &buf.mdata[0], nbytes); */
            for (i=0; i<(int)pr[0]; i++) {
              dat[i] = buf.mdata[i];
            }
/*
        printf(" nbytes:%d \"%d\"\n", nbytes, buf.mdata[0]);
        printf(" id:%d \"%d\"\n", (int)pr[1], buf.mdata[1]);
        printf(" buf.mdata[end]: %d\n", buf.mdata[nbytes-1]);
*/
        
        
        return;
      }
  }

}





/*  MEX GATEWAY  */
void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{

  int num, jobid, i;
  mwSize idx;
  double *arg_p;

  arg_p = mxGetPr(prhs[0]);

  /* TODO: This uses 2x the minimal necessary memory */
 /*  UINT8_T data[(int)arg_p[1]]; */
  UINT8_T data[4096];
  UINT8_T *mesg_data;

  get_shmem_mesg(data, prhs[0], &jobid);

  mesg_data = mxCalloc((int)arg_p[0], sizeof(UINT8_T));
  for ( idx = 0; idx < (int)arg_p[0]; idx++ ) {
    mesg_data[idx] = data[idx];
  }
  plhs[0] = mxCreateNumericMatrix(0, 0, mxUINT8_CLASS, mxREAL);
/*
  printf(" mesg_data[0] : %d \n", mesg_data[0]);
  printf(" mesg_data[end] : %d \n", mesg_data[(int)arg_p[0]-1]);
  printf(" nbytes : %d \n", (int)arg_p[0]);
*/

  /* Point mxArray to dynamicData */
  mxSetData(plhs[0], mesg_data);
  mxSetM(plhs[0], (int)arg_p[0]);
  mxSetN(plhs[0], 1);

  return;
}



