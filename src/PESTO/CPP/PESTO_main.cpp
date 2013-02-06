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
#include <vector>
#include <string>
#include <sstream>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
using namespace std;

#define DEBUG 0
#define MASTER_PORT 11112
#define RUNNING 1
#define KILL    2


typedef struct PestoEnvironment {
  CE_Adaptor *CE;
  Master_Worker_Model *MW;
  int status;
} PestoEnvironment;


void error(const char *msg) {
  perror(msg);
  exit(1);
}
  
void *master_thread(void *PE){
  int ready;
  char *data;
  printf("here\n");
  PestoEnvironment *myPesto;
  myPesto = (PestoEnvironment *)PE;
  cout << "checking..." << myPesto->CE->get_name() << endl;
  myPesto->status = RUNNING;
  myPesto->CE->Launch_CE(myPesto->MW->get_m_cmd(), 0);
  pthread_exit(NULL);
}

void *worker_thread(void *PE){
  int ready;
  char *data;
  printf("here\n");
  PestoEnvironment *myPesto;
  myPesto = (PestoEnvironment *)PE;
  cout << "checking..." << myPesto->CE->get_name() << endl;
  myPesto->status = RUNNING;
  myPesto->CE->Launch_CE(myPesto->MW->get_w_cmd(), 1);
  pthread_exit(NULL);
}

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

// trim from start
static inline string &ltrim(string &s) {
        s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));
        return s;
}

// trim from end
static inline string &rtrim(string &s) {
        s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline string &trim(string &s) {
        return ltrim(rtrim(s));
}

template<typename T, typename P>
T remove_if(T beg, T end, P pred) {
    T dest = beg;
    for (T itr = beg;itr != end; ++itr)
        if (!pred(*itr))
            *(dest++) = *itr;
    return dest;
}

void ReadFile(const char *name, char *outbuff) {
	FILE *file;
  char *buffer;
	unsigned long fileLen;

  cout << "name is " << name << endl;

	//Open file
	file = fopen(name, "rb");
	if (!file)
	{
		fprintf(stderr, "Unable to open file %s", name);
		return;
	}
	
	//Get file length
	fseek(file, 0, SEEK_END);
	fileLen=ftell(file);
	fseek(file, 0, SEEK_SET);

  cout << "length is " << fileLen << endl;

	//Allocate memory
	buffer=(char *)malloc(fileLen+1);
	if (!buffer)
	{
		fprintf(stderr, "Memory error!");
    fclose(file);
		return;
	}

	//Read file contents into buffer
	fread(buffer, fileLen, 1, file);
	fclose(file);

  cout << "file closed" << endl;

  strcpy(outbuff, buffer);
  cout << "string copied" << endl;
  free(buffer);

  cout << "buffer freed " << endl;
}


int main(int argc, char **argv) {

  int rank, size, namelen, i, rc;
  char hostname[MPI_MAX_PROCESSOR_NAME];

  const int CHECKIN_TAG = 1;
  const int DO_WORK_TAG = 2;
  const int KILL_TAG    = 3;
  const int RESULTS_TAG = 4;

  MPI_Init (&argc, &argv);  
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);  
  MPI_Comm_size (MPI_COMM_WORLD, &size);  
  MPI_Get_processor_name(hostname, &namelen);
  cout << "my hostname is " << hostname << endl;


  /* if ( matlab && master_worker ) then: */

  CE_Adaptor *ce = new Matlab_Adaptor();
  Master_Worker_Model *mw = new Master_Worker_Model(ce, hostname, namelen);
  PestoEnvironment *pe;
  pe = (PestoEnvironment *)malloc(sizeof(struct PestoEnvironment));
  memcpy(&pe->CE, &ce, sizeof(ce));
  memcpy(&pe->MW, &mw, sizeof(mw));

  pthread_t t1;

  if (rank==0) {
    int sockfd, newsockfd, portno, n, optval, optlen;
    int firstrun, destination; 
    char *optval2;
    socklen_t clilen;
    char mesg[256];
    struct sockaddr_in serv_addr, cli_addr;
    string protocol;

    /*  This is where I would gather hostnames and make the worker_dict
    for (i=0; i<size; i++) {
      MPI_Recv(data, ...
    }
    */

    pthread_create(&t1, NULL, &master_thread, (void *)pe);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    if (sockfd < 0) 
      error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(MASTER_PORT);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
      error("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    firstrun = 1; 

    while(pe->status==0){
      sleep(1);
    }
    if ( pe->status == RUNNING && firstrun ) {
      while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) 
          error("ERROR on accept");
        else
          break;
      }
    }

    destination = 0;
    vector<int> running_jobs;

    while ( strncmp(mesg,"kill", 4) != 0 ) {

      bzero(mesg,256);
      n = read(newsockfd,mesg,255);
      if (n < 0) error("ERROR reading from socket");
      if ( n > 0 ) {
        printf("       master message is: %s\n",mesg);
        if ( strncmp(mesg,"done", 4) == 0 || strncmp(mesg,"kill", 4) == 0 ) {
          cout << "dead" << endl;
          break;
        }
        
        string message(mesg);

        vector<string> mesg_split = split(message, ',');
        protocol = mesg_split[1];
        /* Clean the string */
        //protocol.erase(remove_if(protocol.begin(), protocol.end(), ::isspace), protocol.end());
        protocol = trim(protocol);

        cout << "protocol : " << protocol << endl;
        if ( protocol == "NETWORK" ) {
            /*  This can wait .... */
        }
        else if ( protocol == "TMPFS" ) {
          string tmpfs_file(trim(mesg_split[3]));
          cout << "tmpfs file is " << tmpfs_file << endl; 
	        FILE *tmpfs_split_file, *tmpfs_shared_file;
	        char *buffer; 
	        unsigned long fileLen;

          /* Read data files */
          // I can't figure out how best to do this:
          //ReadFile(tmpfs_file.c_str(), buffer1);
	        tmpfs_split_file = fopen(tmpfs_file.c_str(), "rb");
	        if (!tmpfs_split_file) error("ERROR reading tmpfs file");
	        
	        //Get file length
	        fseek(tmpfs_split_file, 0, SEEK_END);
	        fileLen=ftell(tmpfs_split_file);
	        fseek(tmpfs_split_file, 0, SEEK_SET);

	        //Allocate memory
	        buffer=(char *)malloc(fileLen+1);
	        if (!buffer) {
            fclose(tmpfs_split_file);
            error ("Memory error!");
          }

	        //Read file contents into buffer
	        fread(buffer, fileLen, 1, tmpfs_split_file);
          message.append(":::::");
          message.append(buffer);
          fclose(tmpfs_split_file);


	        tmpfs_shared_file = fopen("/dev/shm/.jshared.mat", "rb");
          //ReadFile("/dev/shm/.jshared.mat", buffer2);
	        if (!tmpfs_shared_file) error("ERROR reading tmpfs file");
	        
	        //Get file length
	        fseek(tmpfs_shared_file, 0, SEEK_END);
	        fileLen=ftell(tmpfs_shared_file);
	        fseek(tmpfs_shared_file, 0, SEEK_SET);

	        //Allocate memory
	        buffer=(char *)realloc(buffer, fileLen+1);
	        if (!buffer) {
            fclose(tmpfs_shared_file);
            error ("Memory error!");
          }

	        //Read file contents into buffer
	        fread(buffer, fileLen, 1, tmpfs_shared_file);

          message.append(":::::");
          message.append(buffer);
          fclose(tmpfs_shared_file);

          cout << "message is : " << message << endl;
          
        }


        /*  Determine the rank of the next worker  */
        if (destination != size-1)
          destination = (destination + 1) % size;
        else
          destination = 1;

        if ( running_jobs.size() == size-1 ) {
          cout << running_jobs.size() << " JOBS ARE RUNNING" << endl;
          if(DEBUG) 
            cout << "master waiting for a free worker..." << endl;

          //data = mpiComm.recv(source=MPI.ANY_SOURCE, \
          //                    tag=RESULTS_TAG,       \
          //                    status=master_status)

          //if(DEBUG): cout << "GOT EXTRA RESULT:" << endl;

          //running_jobs.remove(master_status.source)

          //if ( protocol == "NETWORK" ){
          //  /* Later ... */
          //}
          //else if ( protocol == "TMPFS" ) {
          //  TMPFS_PATH = '/dev/shm/';
          //  jobid = data[:data.find(':')];
          //  cout << "   JOBID: " << jobid  << " FINISHED" << endl;
          //  results_file = fopen(TMPFS_PATH + '.results_' + jobid + '.mat', "wb");
          //  results_file.fwrite(data[data.find(':')+1:]);
          //  fclose(results_file);
          //  cnn.sendall(results_file.name + '\n');
          //}
          //else {
          //  cnn.sendall(data);
          //}

          //mpiComm.send(mesg, dest=master_status.source, tag=DO_WORK_TAG);
          //running_jobs.append(master_status.source);
          //master_status = MPI.Status();
        } 
        else {
          rc = MPI_Send((void *)message.c_str(), message.length()+1, MPI_CHAR, destination, DO_WORK_TAG, MPI_COMM_WORLD);
          running_jobs.push_back(destination);
        }
      } else 
          continue; 
      

      n = write(newsockfd,"I got your message",18);
      if (n < 0) error("ERROR writing to socket");

    }

    close(newsockfd);
    close(sockfd);

    pthread_exit(NULL);

  }

  else {

    pthread_create(&t1, NULL, &worker_thread, (void *)pe);
    pthread_exit(NULL);

    /*  This is where I would send my worker hostname info to the master  */
    /* data = {'name': name, 'rank': rank}
       mpiComm.send(data, dest=0, tag=CHECKIN_TAG) */



  }

  delete mw;
  delete ce;

  MPI_Finalize();

  return 0;

}
