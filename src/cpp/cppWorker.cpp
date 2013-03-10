#include "computeTask.cpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
using namespace std;

#define WORKER_PORT 11110

vector<string> &split(const string &s, char delim, vector<string> &elems) {
  stringstream ss(s);
  string item;
  while(getline(ss, item, delim)) {
      elems.push_back(item);
  }
  return elems;
}

vector<string> split(const string &s, char delim) {
  vector<string> elems;
  return split(s, delim, elems);
}

int main( int argc, char **argv ) {

  cout << "CPP worker says hello" << endl;

  int sockfd;

  int portno, optval, optlen;
  int n;
  char *optval2;
//  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  int MESGSIZE = 128;
  char mesg[MESGSIZE];
  char *hostname;
  struct hostent *server;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
  if (sockfd < 0) { perror("ERROR opening socket"); exit(-1); }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  hostname = argv[1];
  server = gethostbyname(hostname);
  bcopy((char *)server->h_addr, 
    (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(WORKER_PORT);

    /* connect: create a connection with the server */
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
      perror("ERROR connecting");

//  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
//    perror("ERROR on binding"); exit(-1);
//  }
//  listen(sockfd,5);
//  clilen = sizeof(cli_addr);

//  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
 // if ( newsockfd < 0 ) { perror("ERROR on accept"); exit(-1); }

    bzero(mesg, MESGSIZE);
    strcpy(mesg, "alive");

    /* send the message line to the server */
    n = write(sockfd, mesg, strlen(mesg));
    if (n < 0) 
      perror("ERROR writing to socket");


//  n = write(newsockfd, mesg, strlen(mesg));
//  if (n < 0) { perror("ERROR writing to socket"); exit(-1); }

while (1) {

    bzero(mesg, MESGSIZE);
    n = read(sockfd, mesg, MESGSIZE);
  
  printf("worker got message: %s\n", mesg);

  if ( strncmp(mesg,"kill", 4) == 0 ) {
    cout << "got kill signal" << endl;
    break;
  }

  vector<string> mesg_split;
  string fname, protocol, datafile, jobid, results;
  mesg_split = split(mesg, ',');
  fname = mesg_split[0];
  protocol = mesg_split[1];
  jobid = mesg_split[2];
  datafile = mesg_split[3];

  /* User-defined task function */
  results = computeTask(fname, datafile, jobid);

  cout << "writing results message to socket : " << results << endl;

    bzero(mesg, MESGSIZE);
    strcpy(mesg, results.c_str());

    /* send the message line to the server */
    n = write(sockfd, mesg, strlen(mesg));
    if (n < 0) 
      perror("ERROR writing to socket");

}

  close(sockfd);
  return 0;

}
