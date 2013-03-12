#ifndef __MASTER_H__
#define __MASTER_H__

#include "env.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <boost/archive/tmpdir.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
using namespace std;

#define MASTER_PORT 11112
#define TMPFS_PATH "/dev/shm/"
#define USE_BOOST true

static inline string &ltrim(string &s) {
  s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));
  return s;
}

static inline string &rtrim(string &s) {
  s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());
  return s;
}

static inline string &trim(string &s) {
  return ltrim(rtrim(s));
}


template <class T, class O, unsigned N>
class Master {

  private:
    int sockfd;

  public:
    Master();
    ~Master();
    void Launch();
    void Destroy();
    void SendJobsToWorkers(const char *method, T *, O *);
    void saveObject(T &, const char *);
    void restoreObject(T &, const char *);

};

template <class T, class O, unsigned N>
Master<T, O, N>::Master() {
}


template <class T, class O, unsigned N>
Master<T, O, N>::~Master() {
}

template <class T, class O, unsigned N>
void Master<T, O, N>::Launch() {

  int portno, optval, optlen;
  int n;
  char *optval2;
  struct sockaddr_in serv_addr, cli_addr;

  int MESGSIZE = 128;
  char mesg[MESGSIZE];

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
  if (sockfd < 0) { perror("ERROR opening socket"); exit(-1); }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(MASTER_PORT);

  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
    perror("ERROR connecting");

}

template <class T, class O, unsigned N>
void Master<T, O, N>::Destroy() {
  int n = write(sockfd, "kill\n", strlen("kill\n"));
  if (n < 0) { perror("ERROR writing to socket"); exit(-1); }
  close(sockfd);
}

template <class T, class O, unsigned N>
void Master<T, O, N>::saveObject(T &obj, const char * filename) {
    ofstream ofs(filename);
    boost::archive::text_oarchive oa(ofs);
    oa << obj;
}

template <class T, class O, unsigned N>
void Master<T, O, N>::restoreObject(T &obj, const char * filename) {
    std::ifstream ifs(filename);
    boost::archive::text_iarchive ia(ifs);
    ia >> obj;
}

template <class T, class O, unsigned N>
void Master<T, O, N>::SendJobsToWorkers(const char *method, T *InObj, O *OutObj) {

  int MESGSIZE = 256;
  int n, jobsAccountedFor=0, idxr, idxm, jobid;
  char id[8];
  char filename[64];
  char mesg[MESGSIZE];

  Env env;
  string uid = env.getUid();

  /* Shared data */
  string shared_filename = TMPFS_PATH + ("." + uid) + "_sh.mat";
  ofstream ofsh(shared_filename.c_str(), ios::binary);
  ofsh.write("0", sizeof(int));
  ofsh.close();

  string split_filename_prefix = TMPFS_PATH + ("." + uid) + "_sp_";
  
  for ( int i=0; i<N; i++ ) {
    strcpy(filename, split_filename_prefix.c_str());
    sprintf(id, "%d", i);
    strcat(filename, id);
    strcat(filename, ".bin");

    if (USE_BOOST) {
      cout << "boost..." << endl;
      std::string boostfilename(boost::archive::tmpdir());
      boostfilename = string(filename);
      this->saveObject(InObj[i], boostfilename.c_str());
    }

    else {
      cout << "no boost..." << endl;
      ofstream ofs(filename, ios::binary);
      ofs.write((char *)&InObj[i], sizeof(InObj[i]));
      ofs.close();
    }

    bzero(mesg, MESGSIZE);
    strcpy(mesg, method); 
    strcat(mesg, ",TMPFS,");
    strcat(mesg, id);
    strcat(mesg, ",");
    strcat(mesg, filename);
    strcat(mesg, ",\n");

    n = write(sockfd, mesg, strlen(mesg));
    if (n < 0) { perror("ERROR writing to socket"); exit(-1); }

    memset(filename, '\0', strlen(filename));

  }

  bzero(mesg, MESGSIZE);
  strcpy(mesg, "done"); 
  n = write(sockfd, mesg, strlen(mesg));
  if (n < 0) { perror("ERROR writing to socket"); exit(-1); }

  /* Gather the serialized results and load them */
  while ( jobsAccountedFor < N ) {
    
    bzero(mesg, MESGSIZE);
    n = read(sockfd, mesg, MESGSIZE);

    jobsAccountedFor++;

    string cppmesg = string(mesg);

    idxr = cppmesg.find("_r");
    idxm = cppmesg.find(".mat");
    
    memset(id, '\0', 8*sizeof(char));
    memcpy(id, &mesg[idxr+2], (idxm-idxr-2)*sizeof(char));
    jobid = atoi(id);

    if (USE_BOOST) {
      this->restoreObject(OutObj[jobid], trim(cppmesg).c_str());
    }
    else {
      ifstream ifs(trim(cppmesg).c_str(), ios::binary);
      ifs.read((char *)&OutObj[jobid], sizeof(T));
    }

  }

  return;

}

#endif
