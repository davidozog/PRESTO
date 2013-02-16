#ifndef __NAMESPACE_HM_UTIL__
#define __NAMESPACE_HM_UTIL__

#include <string>
#include <vector>
#include <mpi.h>
#include <map>

using std::string ;
using std::vector ;
using std::map;

namespace HmUtil
{

  double  GetWallTime();
  int     GetCpuSite(int *node, int *core, int *rank);

  // to be removed
  map<string, vector<int> > GetNodesRanks();

  vector<string>            GetNodeOfRank();
  map<string, vector<int> > GetRanksOfNode(const vector<string>& nodes);

  void    CreateMpiComm (vector<int>& commIncRanks, MPI_Comm& subComm );

  void    TrimStrSpaces(string& str);
  string  IntToString(int n);
  bool    IsNumber(const string& str);

  bool    IsDirectory(char path[]);
  int     GetFilesInDirectory(const string& directory, vector<string> &files);
  string  ExtractDirectory(const string& path);
  string  ExtractFileName(const string& path);
  bool    FileExists(const string& strFilename);
  string  GetFileExt(string pathName);


  void    ExitWithError(const string& err);

  double  SwapBytesDouble(double a);
  int     SwapBytesInt(int a);
  void*   SwapBytes(void *a, size_t s);
  bool    IsLittleEndian();

  float   Uniform01();
  float   Uniform(float x1, float x2);

  double  randn_notrig(double mu=0.0, double sigma=1.0);
  double  randn_trig(double mu=0.0, double sigma=1.0);

  int     GetCudaDevicesCount();
  bool    TestCudaDevice(int deviceId);

}

#endif
