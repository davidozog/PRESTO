#include "CE_Adaptor.h"
#include "Matlab_Adaptor.h"
#include "Comp_Model.h"
#include "Master_Worker_Model.h"
#include <iostream>
#include <mpi.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
using namespace std;

#define MASTER_PORT 11112

void error(const char *msg) {
  perror(msg);
  exit(1);
}
  
typedef struct PestoEnvironment {
  CE_Adaptor *CE;
  Master_Worker_Model *MW;
} PestoEnvironment;


void *master_thread(void *PE){
  printf("here\n");
  PestoEnvironment *myPesto;
  myPesto = (PestoEnvironment *)PE;
  cout << "checking..." << myPesto->CE->get_name() << endl;
  myPesto->CE->Launch_CE(myPesto->MW->get_m_cmd());
  pthread_exit(NULL);
}


int main(int argc, char **argv) {

  int rank, size, i;

  MPI_Init (&argc, &argv);  
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);  
  MPI_Comm_size (MPI_COMM_WORLD, &size);  

  /* if ( matlab && master_worker ) then: */

  CE_Adaptor *ce = new Matlab_Adaptor();
  Master_Worker_Model *mw = new Master_Worker_Model(ce);
  PestoEnvironment *pe;
  pe = (PestoEnvironment *)malloc(sizeof(struct PestoEnvironment));
  memcpy(&pe->CE, &ce, sizeof(ce));
  memcpy(&pe->MW, &mw, sizeof(mw));


  if (rank==0) {
    pthread_t t1;
    int sockfd, newsockfd, portno, n;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;

    /*  This is where I would gather hostnames and make the worker_dict
    for (i=0; i<size; i++) {
      MPI_Recv(data, ...
    }
    */

    pthread_create(&t1, NULL, &master_thread, (void *)pe);

//    sockfd = socket(AF_INET, SOCK_STREAM, 0);
//    if (sockfd < 0) 
//      error("ERROR opening socket");
//    bzero((char *) &serv_addr, sizeof(serv_addr));
//    serv_addr.sin_family = AF_INET;
//    serv_addr.sin_addr.s_addr = INADDR_ANY;
//    serv_addr.sin_port = htons(MASTER_PORT);
//    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
//      error("ERROR on binding");
//    listen(sockfd,5);
//    clilen = sizeof(cli_addr);
//    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
//    if (newsockfd < 0) 
//      error("ERROR on accept");
//    bzero(buffer,256);
//    n = read(newsockfd,buffer,255);
//    if (n < 0) error("ERROR reading from socket");
//    printf("Here is the message: %s\n",buffer);
//    n = write(newsockfd,"I got your message",18);
//    if (n < 0) error("ERROR writing to socket");
//    close(newsockfd);
//    close(sockfd);

//    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
//    #sock.settimeout(10)
//    #sock.setblocking(0)  # Sets sockets to non-blocking
//    sock.bind(('localhost', MASTER_PORT))
//    sock.listen(1)
//    firstrun = True


    pthread_exit(NULL);

  }

  else
    ce->Launch_CE(mw->get_w_cmd());

  delete mw;
  delete ce;

  MPI_Finalize();

  return 0;

}
