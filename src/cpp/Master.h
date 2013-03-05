#ifndef __MASTER_H__
#define __MASTER_H__

#include "TestClass.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
using namespace std;

#define MASTER_PORT 11112

template <class T, unsigned N>
class Master {

  private:
    int sockfd, newsockfd;

  public:
    Master();
    ~Master();
    void Launch();
    void Destroy();
    //T* SendJobsToWorkers(const char *method, T(&)[N]);
    void SendJobsToWorkers(const char *method, T *, T *);

};

template <class T, unsigned N>
Master<T, N>::Master() {
}


template <class T, unsigned N>
Master<T, N>::~Master() {
}

template <class T, unsigned N>
void Master<T, N>::Launch() {

  int portno, optval, optlen;
  int n;
  char *optval2;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  char mesg[128];

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
  if (sockfd < 0) { perror("ERROR opening socket"); exit(-1); }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(MASTER_PORT);
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR on binding"); exit(-1);
  }
  listen(sockfd,5);
  clilen = sizeof(cli_addr);

  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if ( newsockfd < 0 ) { perror("ERROR on accept"); exit(-1); }

  strcpy(mesg, "Hello there");

  n = write(newsockfd, mesg, strlen(mesg));
  if (n < 0) { perror("ERROR writing to socket"); exit(-1); }

/*
    String fromClient;
    sock = new Socket("localhost", 11112);
    
    System.out.println("Hello there");
    
    in = new BufferedReader(new InputStreamReader (sock.getInputStream()));
    out = new PrintWriter(sock.getOutputStream(), true);

*/
}

template <class T, unsigned N>
void Master<T, N>::Destroy() {
  close(newsockfd);
  close(sockfd);
}

template <class T, unsigned N>
//T* Master<T, N>::SendJobsToWorkers(const char *method, T (&Obj)[N]) {
void Master<T, N>::SendJobsToWorkers(const char *method, T *InObj, T *OutObj) {

  cout << "N is " << N << endl;
  int n;
  char id[8];
  char filename[64];
  char mesg[256];

  
  for ( int i=0; i<N; i++ ) {
    strcpy(filename, "/dev/shm/.cv_");
    sprintf(id, "%d", i);
    strcat(filename, id);
    strcat(filename, ".bin");
    cout << "filename:" << filename << endl;
    ofstream ofs(filename, ios::binary);
    ofs.write((char *)&InObj[i], sizeof(InObj[i]));

    strcpy(mesg, method); 
    strcat(mesg, ",TMPFS,");
    strcat(mesg, id);
    strcat(mesg, ",");
    strcat(mesg, filename);
    strcat(mesg, ",");

    cout << "mesg is: " << mesg << endl;

    n = write(newsockfd, mesg, strlen(mesg));
    if (n < 0) { perror("ERROR writing to socket"); exit(-1); }

    memset(filename, '\0', strlen(filename));
    memset(mesg, '\0', strlen(mesg));

  }

//     out.println(mesg);
//
//   }
//
//   out.println("done");
//
//   String fromMPI, strIdx;
//   int idx;
//
//
//   /* Wait for all the results */
//   int jobsAccountedFor = 0;
//   
//   while ( jobsAccountedFor < numTasks ) {
//
//     try {
//
//       fromMPI = in.readLine();
//     
//     }  catch (Exception e) {
//        continue;
//     }
//
//     System.out.println("fromMPI is:" + fromMPI);
//     if( fromMPI!= null && !fromMPI.isEmpty() ) {
//       jobsAccountedFor = jobsAccountedFor + 1;
//       System.out.println ("Got string " + fromMPI);
//
//       /* De-serialize the data file */
//       FileInputStream f_in = new FileInputStream (fromMPI);
//       ObjectInputStream obj_in = new ObjectInputStream (f_in);
//       Object obj = obj_in.readObject();
//       
//       /* Get job ID from filename */
//       String[] splits = fromMPI.split("_");
//       System.out.println("this is :" + splits[1].substring(0, splits[1].indexOf(".")));
//       strIdx = splits[1].substring(0, splits[1].indexOf("."));
//       idx = Integer.parseInt(strIdx);
//
//       Obj[idx] = (T)obj;
//
//     }
//
//   }
//
//   return Obj;
//

  for (int i=0; i<N; i++) {
    OutObj[i].TestKernel(&InObj[i]);
  }

}

#endif
