#include <string>
#include <string.h>
#include <sys/time.h>
#include <iostream>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h> 
#include <cmath>
#include <sstream>
#include <algorithm>
#include "HmUtil.h"

using std::cerr;
using std::endl;
using std::string;
using std::log;
using std::ostringstream;
using std::cout;
using std::endl;
using std::sort;


#ifdef CUDA_ENABLED

extern "C" int get_cuda_device_count();
extern "C" bool test_cuda_device(int);

#endif


void HmUtil::TrimStrSpaces(string& str){

  size_t startpos = str.find_first_not_of(" \t"); 
  size_t endpos = str.find_last_not_of(" \t"); 

  if(( string::npos == startpos ) || ( std::string::npos == endpos)) str = "";
  else str = str.substr( startpos, endpos-startpos+1 );
}

string HmUtil::GetFileExt(string pathName) {
  int period = pathName.find_last_of(".");
  if (period < 0) return "";
  string ext = pathName.substr(period + 1);
  TrimStrSpaces(ext);
  return  ext;
}

/*
bool HmUtil::IsNumber(const std::string& str){

  string s = str;
  HmUtil::TrimStrSpaces(s);

  for (int i = 0; i < s.length(); i++) {
    if (!std::isdigit(s[i]))
      return false;
  }

  return true;

}
*/

bool HmUtil::IsNumber(const std::string& str){

  string s = str;
  HmUtil::TrimStrSpaces(s);

  int numDots = 0;

  for (int i = 0; i < s.length(); i++) {
    if (s[i] == '.') {
      if (numDots > 0) return false;
      else numDots++;
    }
    else if (!std::isdigit(s[i])) 
      return false;
  }
  return true;
}



string HmUtil::IntToString( int n ) {

  ostringstream result;
  result << n;
  return result.str();

}

double HmUtil::GetWallTime()
{
    struct timeval tp;
    int rtn = gettimeofday(&tp, NULL);
    return ((double)tp.tv_sec+(1.e-6)*tp.tv_usec);
}

void HmUtil::ExitWithError(const string& err ){
  cerr << err << endl;				
  exit(1);
}

float HmUtil::Uniform01(){
  return ((float) rand())/(float)RAND_MAX;
}

float HmUtil::Uniform(float x1, float x2){
  return ((double) Uniform01()*(x2-x1)+x1);
}

/***
 * A function to check if a path is a file or a directory
 * Author: Danny Battison
 * Contact: gabehabe@gmail.com
 * http://www.dreamincode.net/code/snippet2699.htm
 * This function slitely modified by adnan
 */

bool HmUtil::IsDirectory(char path[]) {
    int i = strlen(path) - 1;
    if (path[strlen(path)] == '.') {return true;} // exception for directories
    // such as \. and \..
    for(i; i >= 0; i--) {
        if (path[i] == '.') return false; // if we first encounter a . then it's a file
        else if (path[i] == '\\' || path[i] == '/') return true; // if we first encounter a \ it's a dir
    }
}

int HmUtil::GetFilesInDirectory(const string& dir, vector<string> &files){
  DIR *dp;
  struct dirent *dirp;
  if((dp  = opendir(dir.c_str())) == NULL) {
    cerr << "IOError: opening dir (" << errno << ") " << dir << endl;
    return errno;
  }  

  while ((dirp = readdir(dp)) != NULL) 
    files.push_back(string(dirp->d_name));
  closedir(dp);
  return 0;
}

string HmUtil::ExtractDirectory(const string& path){
  return path.substr(0, path.find_last_of("//") + 1);
}

string HmUtil::ExtractFileName(const string& path){
  return path.substr(path.find_last_of("//") + 1);
}

//this function is taken from 
//http://www.techbytes.ca/techbyte103.html
bool HmUtil::FileExists(const string& strFilename) { 
  struct stat stFileInfo; 
  bool blnReturn; 
  int intStat; 

  // Attempt to get the file attributes 
  intStat = stat(strFilename.c_str(),&stFileInfo); 
  if(intStat == 0) { 
    // We were able to get the file attributes 
    // so the file obviously exists. 
    blnReturn = true; 
  } else { 
    // We were not able to get the file attributes. 
    // This may mean that we don't have permission to 
    // access the folder which contains this file. If you 
    // need to do that level of checking, lookup the 
    // return values of stat which will give you 
    // more details on why stat failed. 
    blnReturn = false; 
  } 
   
  return(blnReturn); 
}

//not tested 
double  HmUtil::SwapBytesDouble(double a){

  double b;
  unsigned char *from = (unsigned char *)& a;
  unsigned char *to   = (unsigned char *)& b;
  
  to[0] = from[7];
  to[1] = from[6];
  to[2] = from[5];
  to[3] = from[4];
  to[4] = from[3];
  to[5] = from[2];
  to[6] = from[1];
  to[7] = from[0];

  return b;
}

void* SwapBytes(void *a, size_t s){
  char *front = (char *) a;
  char *back  = front + s - 1;

  for (; front > back; --back, ++front) {
    char temp = *front;
    *front = *back;
    *back = temp;
  }
  return a;
}


int  HmUtil::SwapBytesInt(int a){

  int b;
  unsigned char *from = (unsigned char *)& a;
  unsigned char *to   = (unsigned char *)& b;
  
  to[0] = from[3];
  to[1] = from[2];
  to[2] = from[1];
  to[3] = from[0];
  return b;
}


bool HmUtil::IsLittleEndian(){
  int n = 1;
  return ((*(char *)&n == 1) ? true : false);
}

int HmUtil::GetCudaDevicesCount(){
  
#ifdef CUDA_ENABLED
  return get_cuda_device_count();
#else
  return 0;
#endif 
}

bool HmUtil::TestCudaDevice(int deviceId){
  
#ifdef CUDA_ENABLED
  return test_cuda_device(deviceId);
#else
  return false;
#endif 

}

// returns the node of a rank 
vector<string> HmUtil::GetNodeOfRank(){
  
  // note: couldn't get stl string working with MPI, 
  // using c strings instead 

  char  host_name[MPI_MAX_PROCESSOR_NAME];
  char (*host_names)[MPI_MAX_PROCESSOR_NAME];

  int nprocs, namelen, rank;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Get_processor_name(host_name,&namelen);

  int bytes = nprocs * sizeof(char[MPI_MAX_PROCESSOR_NAME]);
  host_names = (char (*)[MPI_MAX_PROCESSOR_NAME]) malloc(bytes);
  strcpy(host_names[rank], host_name);
  
  MPI_Allgather(&(host_names[rank]), MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 
		host_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,  MPI_COMM_WORLD);

  // now use strings
  vector<string > nodes(nprocs);
  for (int i=0; i<nprocs; i++)
    nodes[i] = host_names[i];

  return nodes;

}


map<string, vector<int> > HmUtil::GetRanksOfNode(const vector<string>& nodes){
  
  map<string, vector<int> > ranksOfNode;
  for (int i=0; i<nodes.size(); i++)
    ranksOfNode[nodes[i]].push_back(i);

  // sort them
  map<string, vector<int> >::iterator it = ranksOfNode.begin();
  for (; it != ranksOfNode.end(); it++){
    sort((it->second).begin(), (it->second).end());
  }
  return ranksOfNode;

}

// This function return a map of the ranks in nodes 
// Keys are nodes names, elemennts are vectors contain 
// all ranks in that node sorted 
map<string, vector<int> >  HmUtil::GetNodesRanks(){
  
 // get hostnames of ranks 
  char  host_name[MPI_MAX_PROCESSOR_NAME];
  char (*host_names)[MPI_MAX_PROCESSOR_NAME];

  int nprocs, namelen, rank;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Get_processor_name(host_name,&namelen);

  int bytes = nprocs * sizeof(char[MPI_MAX_PROCESSOR_NAME]);

  host_names = (char (*)[MPI_MAX_PROCESSOR_NAME]) malloc(bytes);
  strcpy(host_names[rank], host_name);
  
  for (int n=0; n<nprocs; n++) {
    MPI_Bcast(&(host_names[n]),MPI_MAX_PROCESSOR_NAME, MPI_CHAR, n, 
	      MPI_COMM_WORLD); 
  }

  map<string, vector<int> > nodes;
  for (int i=0; i<nprocs; i++){
    string rankNode(host_names[i]);
    TrimStrSpaces(rankNode);
    nodes[rankNode].push_back(i);
  }

  // sort them
  map<string, vector<int> >::iterator it = nodes.begin();
  for (; it != nodes.end(); it++){
    sort((it->second).begin(), (it->second).end());
  }
}

void HmUtil::CreateMpiComm (vector<int>& commIncRanks, MPI_Comm& subComm ) {

  MPI_Group worldGroup, subGroup;
  MPI_Comm  world = MPI_COMM_WORLD;

  MPI_Comm_group(world, &worldGroup);
  MPI_Group_incl(worldGroup, (int) commIncRanks.size(), 
		 &commIncRanks[0], &subGroup);
  int retv =  MPI_Comm_create(world, subGroup, &subComm);

}


/*! \todo remove this unused function  */

int HmUtil::GetCpuSite(int *node, int *core, int *rank){
  
  char host_name[MPI_MAX_PROCESSOR_NAME];
  char (*host_names)[MPI_MAX_PROCESSOR_NAME];
  MPI_Comm internode;
  MPI_Comm intranode;
  int nprocs, namelen,n,bytes;

  MPI_Comm_rank(MPI_COMM_WORLD, (int*)rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Get_processor_name(host_name,&namelen);
  bytes = nprocs * sizeof(char[MPI_MAX_PROCESSOR_NAME]);

  host_names = (char (*)[MPI_MAX_PROCESSOR_NAME]) malloc(bytes);
  strcpy(host_names[*rank], host_name);
  for (n=0; n<nprocs; n++)
    {
      MPI_Bcast(&(host_names[n]),MPI_MAX_PROCESSOR_NAME, MPI_CHAR, n, MPI_COMM_WORLD); 
    }
	
  unsigned int color;
  color = 0;
  for (n=1; n<nprocs; n++)
    {
      if(strcmp(host_names[n-1], host_names[n])) color++;
      if(strcmp(host_name, host_names[n]) == 0) break;
    }
	
  MPI_Comm_split(MPI_COMM_WORLD, color, *rank, &internode);
  MPI_Comm_rank(internode,(int*)core);
  
  MPI_Comm_split(MPI_COMM_WORLD, *core, *rank, &intranode);
  
  MPI_Comm_rank(intranode,(int*)node);
  return 0;
}

////this function is taken from:  http://www.dreamincode.net/code/snippet1446.htm
/* randn()
 * 
 * Normally (Gaussian) distributed random numbers, using the Box-Muller 
 * transformation.  This transformation takes two uniformly distributed deviates
 * within the unit circle, and transforms them into two independently 
 * distributed normal deviates.  Utilizes the internal rand() function; this can
 * easily be changed to use a better and faster RNG.
 * 
 * The parameters passed to the function are the mean and standard deviation of 
 * the desired distribution.  The default values used, when no arguments are 
 * passed, are 0 and 1 - the standard normal distribution.
 * 
 * 
 * Two functions are provided:
 * 
 * The first uses the so-called polar version of the B-M transformation, using
 * multiple calls to a uniform RNG to ensure the initial deviates are within the
 * unit circle.  This avoids making any costly trigonometric function calls.
 * 
 * The second makes only a single set of calls to the RNG, and calculates a 
 * position within the unit circle with two trigonometric function calls.
 * 
 * The polar version is generally superior in terms of speed; however, on some
 * systems, the optimization of the math libraries may result in better 
 * performance of the second.  Try it out on the target system to see which
 * works best for you.  On my test machine (Athlon 3800+), the non-trig version
 * runs at about 3x10^6 calls/s; while the trig version runs at about
 * 1.8x10^6 calls/s (-O2 optimization).
 * 
 * 
 * Example calls:
 * randn_notrig();//returns normal deviate with mean=0.0, std. deviation=1.0
 * randn_notrig(5.2,3.0);//returns deviate with mean=5.2, std. deviation=3.0
 * 
 * 
 * Dependencies - requires <cmath> for the sqrt(), sin(), and cos() calls, and a
 * #defined value for PI.
 */

/******************************************************************************/
//"Polar" version without trigonometric calls

double HmUtil::randn_notrig(double mu, double sigma) {
  static bool deviateAvailable=false;//flag
  static float storedDeviate;//deviate from previous calculation
  double polar, rsquared, var1, var2;
  
  //If no deviate has been stored, the polar Box-Muller transformation is 
  //performed, producing two independent normally-distributed random
  //deviates.  One is stored for the next round, and one is returned.
  if (!deviateAvailable) {
    
    //choose pairs of uniformly distributed deviates, discarding those 
    //that don't fall within the unit circle
    do {
      var1=2.0*( double(rand())/double(RAND_MAX) ) - 1.0;
      var2=2.0*( double(rand())/double(RAND_MAX) ) - 1.0;
      rsquared=var1*var1+var2*var2;
    } while ( rsquared>=1.0 || rsquared == 0.0);
    
    //calculate polar tranformation for each deviate
    polar=sqrt(-2.0*log(rsquared)/rsquared);
    
    //store first deviate and set flag
    storedDeviate=var1*polar;
    deviateAvailable=true;
    
    //return second deviate
    return var2*polar*sigma + mu;
  }
  
  //If a deviate is available from a previous call to this function, it is
  //returned, and the flag is set to false.
  else {
    deviateAvailable=false;
    return storedDeviate*sigma + mu;
  }
}


/******************************************************************************/
//Standard version with trigonometric calls

#define PI 3.14159265358979323846

double HmUtil::randn_trig(double mu, double sigma) {
  static bool deviateAvailable=false;//flag
  static float storedDeviate;//deviate from previous calculation
  double dist, angle;
  
  //If no deviate has been stored, the standard Box-Muller transformation is 
  //performed, producing two independent normally-distributed random
  //deviates.  One is stored for the next round, and one is returned.
  if (!deviateAvailable) {
    
    //choose a pair of uniformly distributed deviates, one for the
    //distance and one for the angle, and perform transformations
    dist=sqrt( -2.0 * log(double(rand()) / double(RAND_MAX)) );
    angle=2.0 * PI * (double(rand()) / double(RAND_MAX));
    
    //calculate and store first deviate and set flag
    storedDeviate=dist*cos(angle);
    deviateAvailable=true;
    
    //calcaulate return second deviate
    return dist * sin(angle) * sigma + mu;
  }
  
  //If a deviate is available from a previous call to this function, it is
  //returned, and the flag is set to false.
  else {
    deviateAvailable=false;
    return storedDeviate*sigma + mu;
  }
}

