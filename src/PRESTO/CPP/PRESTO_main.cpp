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
#include <list>
#include <string>
#include <sstream>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <queue>
using namespace std;

#define DEBUG 0
#define MASTER_PORT 11112
#define WORKER_PORT 11110
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
  MPI_Status stat;

  /* if ( matlab && master_worker ) then: */

  CE_Adaptor *ce = new Matlab_Adaptor();
  Master_Worker_Model *mw = new Master_Worker_Model(ce, hostname, namelen);
  PestoEnvironment *pe;
  pe = (PestoEnvironment *)malloc(sizeof(struct PestoEnvironment));
  memcpy(&pe->CE, &ce, sizeof(ce));
  memcpy(&pe->MW, &mw, sizeof(mw));

  pthread_t t1;
  int sockfd, newsockfd, portno, optval, optlen;
  int n, firstrun, destination; 
  char *optval2;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  vector<string> mesg_split;
  string protocol, jobid;
	char *split_buff, *shared_buff; 
	int fileLen_split, fileLen_shared, fileLen;
	char *buffer; 


  if (rank==0) {
   char mesg[4096];
   int fromq;
   string message;
   n=0;

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
   queue<string> task_q;
   task_q.push("run");

   while ( 1 ) {
    if ( !task_q.empty() ) {
      message = task_q.front();
      task_q.pop();
    }
    if ( message == "run" && firstrun ) 
      newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if ( newsockfd < 0 )
      error("ERROR on accept");

    destination = 0;
    list<int> running_jobs;

    while ( strncmp(mesg,"kill", 4) != 0 ) {

      bzero(mesg, 4096);

      if ( !task_q.empty() ) {
        cout << "fromq" << endl;
        message = task_q.front();
        task_q.pop();
        strcpy(mesg, message.c_str());
        fromq = 1;
        n = strlen(mesg)*sizeof(char);
      }
      else {
        cout << "NOT fromq" << endl;
        cout << "mesg is first :" << mesg << endl;
        n = read(newsockfd, mesg, 4095);
        cout << "mesg is now :" << mesg << endl;
        message = mesg;
        fromq = 0;
      }
      if ( n < 0 ) error("ERROR reading from socket");
      if ( n > 0 && !fromq ) {
        mesg_split = split(message, '\n');
        cout << "I'm dealing with " << mesg_split.size() << " messages" << endl;
        if ( mesg_split.size() > 1 ) 
          for (i=0; i<mesg_split.size(); i++) {
            message = mesg_split[i];
            task_q.push(message);
          }
        else {
          fromq = 1;
        }
      }
      if ( n > 0 && fromq ) {

        printf("       master message is: %s\n",mesg);
        if ( strncmp(mesg,"done", 4) == 0 || strncmp(mesg,"kill", 4) == 0 ) {
          cout << "dead" << endl;
          break;
        }
        
        string message(mesg);

        mesg_split = split(message, ',');
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

          /* Read data files */
          // I can't figure out how best to do this:
          //ReadFile(tmpfs_file.c_str(), buffer1);
	        tmpfs_split_file = fopen(tmpfs_file.c_str(), "rb");
	        if (!tmpfs_split_file) error("ERROR reading tmpfs file");
	        
	        //Get file length
	        fseek(tmpfs_split_file, 0, SEEK_END);
	        fileLen_split = ftell(tmpfs_split_file);
	        fseek(tmpfs_split_file, 0, SEEK_SET);

	        //Allocate memory
	        split_buff = (char *)malloc(fileLen_split+1);
	        if ( !split_buff ) {
            fclose(tmpfs_split_file);
            error ("Memory error!");
          }

	        //Read file contents into buffer
	        fread(split_buff, fileLen_split, 1, tmpfs_split_file);
          //message.append("\n");
          //message.append(split_buff);
          fclose(tmpfs_split_file);


	        tmpfs_shared_file = fopen("/dev/shm/.jshared.mat", "rb");
          //ReadFile("/dev/shm/.jshared.mat", buffer2);
	        if (!tmpfs_shared_file) error("ERROR reading tmpfs file");
	        
	        //Get file length
	        fseek(tmpfs_shared_file, 0, SEEK_END);
	        fileLen_shared=ftell(tmpfs_shared_file);
	        fseek(tmpfs_shared_file, 0, SEEK_SET);

	        //Allocate memory
	        shared_buff = (char *)malloc(fileLen_shared+1);
	        if (!shared_buff) {
            fclose(tmpfs_shared_file);
            error ("Memory error!");
          }

	        //Read file contents into buffer
	        fread(shared_buff, fileLen_shared, 1, tmpfs_shared_file);

          //message.append("\n");
          //message.append(buffer);
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
          // TODO: It would probably be best to make these non-blocking
          rc = MPI_Send((void *)message.c_str(), message.length()+1, MPI_CHAR, destination, DO_WORK_TAG, MPI_COMM_WORLD);
          rc = MPI_Send(&fileLen_split, 1, MPI_INT, destination, DO_WORK_TAG, MPI_COMM_WORLD);
          rc = MPI_Send((void *)split_buff, fileLen_split, MPI_CHAR, destination, DO_WORK_TAG, MPI_COMM_WORLD);
          rc = MPI_Send(&fileLen_shared, 1, MPI_INT, destination, DO_WORK_TAG, MPI_COMM_WORLD);
          rc = MPI_Send((void *)shared_buff, fileLen_shared, MPI_CHAR, destination, DO_WORK_TAG, MPI_COMM_WORLD);
          running_jobs.push_back(destination);

          free(split_buff);
          free(shared_buff);
          
        }
      } else 
          continue; 
      
      //n = write(newsockfd,"I got your message",18);
      //if (n < 0) error("ERROR writing to socket");

    }
    /*  Wait for job completion  */
    while ( running_jobs.size() > 0 ) {
      cout << running_jobs.size() << " JOBS ARE RUNNING" << endl;
      if(DEBUG)  cout << "master waiting for results..." << endl;
      data = mpiComm.recv(source=MPI.ANY_SOURCE, tag=RESULTS_TAG, status=master_status)
      rc = MPI_Recv(&fileLen, 1, MPI_INT, MPI_ANY_SOURCE, RESULTS_TAG, 
                    MPI_COMM_WORLD, &stat);
      
	    buffer=(char *)malloc(fileLen+1);
      rc = MPI_Recv(buffer, fileLen, MPI_CHAR, stat.MPI_SOURCE, RESULTS_TAG, 
                    MPI_COMM_WORLD, &stat);

      if(DEBUG) cout << "GOT RESULT:" << buffer << endl;
      running_jobs.remove(stat.MPI_SOURCE)

      if ( protocol == "NETWORK") {
        /*  Later ...  */
      }
      else if ( protocol == "TMPFS" ) {
        TMPFS_PATH = "/dev/shm/";
        jobid = data[:data.find(':')]
        print '   JOBID: ' + jobid + ' FINISHED'
        results_file = open(TMPFS_PATH + '.results_' + jobid + '.mat', 'wb')
        results_file.write(data[data.find(':')+1:])
        results_file.close()
        cnn.sendall(results_file.name + '\n')
      }
    
/*
      else:
        cnn.sendall(data)
*/
    }

    close(newsockfd);
    close(sockfd);

    pthread_exit(NULL);

   }
  }

  else {
    char mesg[4096];
    string results_filename;
	  FILE *results_file;

    pthread_create(&t1, NULL, &worker_thread, (void *)pe);

    /*  This is where I would send my worker hostname info to the master  */
    /* data = {'name': name, 'rank': rank}
       mpiComm.send(data, dest=0, tag=CHECKIN_TAG) */

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    if ( sockfd < 0 ) 
      error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(WORKER_PORT);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
      error("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if ( newsockfd < 0 ) 
      error("ERROR on accept");
    else {
      cout << "Worker connected: " << cli_addr.sin_addr.s_addr << endl;
    }

    bzero(mesg, 4096);
    n = read(newsockfd, mesg, 4096);

    string message(mesg);

    cout << "mesg is " << message << endl;

    while ( strncmp(mesg,"kill", 4) != 0 ) {

      // TODO:  Maybe I could get message size first
      rc = MPI_Recv(mesg, 4096, MPI_CHAR, 0, MPI_ANY_TAG, 
                    MPI_COMM_WORLD, &stat);
      cout << "mesg :" << mesg << endl;
      rc = MPI_Recv(&fileLen_split, 1, MPI_INT, 0, MPI_ANY_TAG, 
                    MPI_COMM_WORLD, &stat);
      cout << "fileLen_split :" << fileLen_split << endl;
	    split_buff = (char *)malloc(fileLen_split);
      rc = MPI_Recv(split_buff, 4096, MPI_CHAR, 0, MPI_ANY_TAG, 
                    MPI_COMM_WORLD, &stat);
      cout << "split_buff :" << split_buff << endl;
      rc = MPI_Recv(&fileLen_shared, 1, MPI_INT, 0, MPI_ANY_TAG, 
                    MPI_COMM_WORLD, &stat);
      cout << "fileLen_shared :" << fileLen_shared << endl;
	    shared_buff = (char *)malloc(fileLen_shared);
      rc = MPI_Recv(shared_buff, 4096, MPI_CHAR, 0, MPI_ANY_TAG, 
                    MPI_COMM_WORLD, &stat);
      cout << "shared_buff :" << shared_buff << endl;

      message = mesg;

      mesg_split = split(message, ',');
      protocol = mesg_split[1];
      protocol = trim(protocol);
      jobid = mesg_split[2];
      jobid = trim(jobid);

      if ( stat.MPI_TAG == DO_WORK_TAG ) {
        cout << "GOT IT!" << endl;
        cout << "protocol is " << protocol << endl;
        if ( protocol == "NETWORK" ) {
          /*  Later ...  */
        }
        else if ( protocol == "TMPFS" ) {
          string tmpfs_file(trim(mesg_split[3]));
	        FILE *tmpfs_split_file, *tmpfs_shared_file;

	        tmpfs_split_file = fopen(tmpfs_file.c_str(), "wb");
	        if (!tmpfs_split_file) error("ERROR reading tmpfs split file");
          fwrite((const void *)split_buff, sizeof(char), fileLen_split, 
                 tmpfs_split_file);
          fclose(tmpfs_split_file);

          tmpfs_shared_file = fopen("/dev/shm/.jshared.mat", "wb");
	        if (!tmpfs_shared_file) error("ERROR reading tmpfs shared file");
          fwrite((const void *)shared_buff, sizeof(char), fileLen_shared, 
                 tmpfs_shared_file);
          fclose(tmpfs_shared_file);

          cout << "Worker files written" << endl;
          if ( mesg[strlen(mesg)-1] != '\n' )
            strcat(mesg, "\n");
          cout << "sending mesg :" << mesg << " with length " << strlen(mesg) << endl;
          n = write(newsockfd, mesg, strlen(mesg));
          if (n < 0) error("ERROR writing to socket");
          cout << "sent!" << endl;

        }
        else {
          n = write(newsockfd, mesg, strlen(mesg));
          if (n < 0) error("ERROR writing to socket");
        }

      /*  Wait for results (filename):  */
      bzero(mesg, 256);
      n = read(newsockfd, mesg, 256);
      if(DEBUG) cout << "W:" << rank << ":(finished/received)" << endl;

      if ( protocol == "NETWORK" ) {
          /*  Later ... */
      }

      else if ( protocol == "TMPFS" ) {
        if(DEBUG)  cout <<  "results file is"  << mesg << endl;
        results_filename = mesg;
        results_filename = trim(results_filename);
	      results_file = fopen(results_filename.c_str(), "wb");

	      if (!results_file) error("ERROR reading tmpfs results file");
	      fseek(results_file, 0, SEEK_END);
	      fileLen=ftell(results_file);
	      fseek(results_file, 0, SEEK_SET);

        cout << "length is " << fileLen << endl;

	      buffer=(char *)malloc(fileLen+1);
	      if (!buffer) {
	      	error("Memory error!");
          fclose(results_file);
	      }
	      fread(buffer, fileLen, 1, results_file);
        fclose(results_file);
      }

      strcpy(mesg, jobid.c_str());
      strcat(mesg, ":");
      strcat(mesg, buffer);
      free(buffer);

      cout << "W:" << rank << ": sending back mesg :" << mesg << endl;

      /*  Send results (filename) back to master:  */
      rc = MPI_Send((void *)mesg, fileLen+jobid.length()+1, MPI_CHAR, 0, RESULTS_TAG, MPI_COMM_WORLD);

/*
      # See if the master has decided to die:
      #mpiComm.Iprobe(source=0, tag=KILL_TAG, status=worker_status)
      #if (worker_status.tag == KILL_TAG):
      #  if(DEBUG):print 'KILL MESSAGE came from ' + str(worker_status.source)
      #  kill = mpiComm.recv(source=worker_status.source, tag=worker_status.tag)
*/



      }
      else if ( stat.MPI_TAG == KILL_TAG ){
/*
        if protocol == 'NETWORK':
          semaphore.acquire()
          memory.unlink()
          mapfile.close()

        conn.sendall("kill")
        conn.close()
        break
*/
      }

    }

    pthread_exit(NULL);

  }

  delete mw;
  delete ce;

  MPI_Finalize();

  return 0;

}
