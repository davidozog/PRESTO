/*! \class CondInv
* \brief Conductivity optimization using simplex or simulated annealing algorithms 
* \author Adnan Salman
* \author NeuroInformatic Center
* \date 2012
*/


#ifndef __COND_INV__
#define __COND_INV__

#include <vector>
#include <map>
#include "CondObjFunc.h"
#include "simplex.h"
#include "paral_sa.h"
#include "Poisson.h"

using std::vector;
using std::map;

/*! Enumeration of MPI process task */
enum ProcessTask { MASTER, /*!< Master process */ 
		   WORKER, /*!< Worker process */
		   NOTHING /*!< Rendundant should return without consuming resources */
};

class CondInv {
 private:

  //! Forward solver
  Poisson                     *mpPoissonSolver;

  //! Forward solver algorithm
  string                       mPoissonSolverAlgorithm;

  //! All tissues initial conductivity values 
  vector<float>                mInitConds;  

  //! Variable tissue conductivities  
  vector<float>                mVarTissuesInitConds;

  //! Variable tissue conductivities lower bound  
  vector<float>                mTissueLowerBound;

  //! Variable tissue conductivities upper bound  
  vector<float>                mTissueUpperBound;

  //! Indices of variable tissues in the all tissues vector
  vector<int>                  mVarTissuesIdx;

  //! 3 elements vector contains the location of the reference electrode
  vector<int>                  mRefElectrodePos;

  //! Input parameters from the hm input file
  map<string, vector<string> > mInputParams;

  //! Optimization method to use (simplex, sa)
  string                       mOptimMethName;

  //! Optimization method object
  OptimMethod                 *mpOptimizer;

  //! Seed for random number generator
  unsigned int                 mRandSeed;

  //! The objective function method to use (l2norm, etc) 
  string                       mObjFuncMeth;

  //! Objective function object
  CondObjFunc                 *mpObjFunc;

  //! The data path 
  string                       mDataPath;

  //! TOBE removed now in the HeadModel class 
  string                       mGeomFileName;
  //! TOBE removed now in the HeadModel class 
  string                       mSnsFileName;
  //! TOBE removed now in the HeadModel class 
  string                       mBrainkDataDir;
  //! TOBE removed now in the HeadModel class 
  string                       mBrainkDataKind;

  //! when using anisotropic model, last two parameters are used for anisotropy
  int                          mNumIsoTissues;
  bool                         mAnisotropicModel;

  //! Process rank
  int                          mRank;

  //! parallelism (omp or cuda )
  string                       mCompMode;

  //! MPI communicator used in the optimization (subset of world)
  MPI_Comm                     mMpiComm;

  //! The process kind of work to do (master or worker)
  ProcessTask                  mMyTask;

  //! Initialized the CondInv object with default values 
  int  Init_();

  //! Instantiate forward solver  object mpPoissonSolver
  int  CreateForwardSolver();

  //! From input file sets which tissues to vary 
  int  SetVariableTissues();

  //! Sets the simulated annealing parameters as specified in the input file
  void SetSimAnnParams();

  //! Sets the simplex parameters as specified in the input file
  void SetSimplexParams();

  /*! Starts the inverse optimization  
    \param [in] measuredFileName EIT current injection data file see CurrentInjecData class
    \param [in] outs output stream to print optimization results in the output file (only master)  
    \param [in] rProcessOutStream output stream that print the optimization progress of each process in files appened with _(rank)
   */
  void Inverse(const string& measured_file_name, ostream& outs, ostream& rProcessOutStream);


  /*! Creates MPI communicator from the world communicator   
    \param [in, out] forwardParallelism The forward solver parallelism (mpi, cuda or auto). 
                     If auto the parameter will be set to the appropriate parallelism
    \param [out] deviceIdOrOmpThreads returns the cuda device id in case of cuda parallelism 
                                       or the OMP number threads incase omp paralleism 
    \param [in]  optimMethodName optimization method used, optimal choices of processes 
                                  geometry can improve performance. In case of simulated annealing, ns parameter should 
				  be a multiple of the number of processes 
    \param [in]  ns simulated annealing intermediate loop

    \return Whether the calling process is included in the new communicator or not
   */
  bool MakeComunicator (string& forwardParallelism, int& deviceIdOrOmpThreads, 
			       MPI_Comm& cond_comm1, int ns, 
			       const string& optimMethodName  );

  void FilterWorkers (const string& optimMethodName, 
		      vector<ProcessTask>& ranksProposedWork, 
		      vector<int>& commRanks, 
		      map<string, vector<int> >& commNodesRanks,
		      vector<string>& hostNames, int ns);


 public:

  //! default
  CondInv();

  //! default provided the process rank
  CondInv(int rank);

  //! default provided the input parameters 
  CondInv(map<string, vector<string> > params, int rank);

  ~CondInv();

  //! Initilization given parameters from the input file
  int  Init(map<string, vector<string> > params);

  /*! Finds optimal conductivity values that minimizes the objective function of the 
  difference between the computed and the EIT data for possibly multiple EIT files 
  \param [in] measured_data either directory of EIT data files or a single EIT data file
  */
  void Optimize(string& measured_data);

  /*! Finds optimal conductivity values that minimizes the objective function of the 
  difference between the computed and the EIT data for a single EIT file 
  \param [in] rMeasuredData EIT data file
  */
  void OptimizeFile(const string& rMeasuredData);

  /*! Writes optimization results  
  \param [in] outs output stream
  \param [in] optimalConds final optimal conductivity values 
  \param [in] initial conductivity values initConds
  \param [in] objFunOptimValue objective function corresponds to optimal conductivities 
  \param [in] returnCode which termination condition is satisfied 
  \param [in] numberFcnEval number of objective function evaluations performed 
  */
  void WriteResults(ostream& outs, const vector<float> &optimalConds, 
		    const vector<float> &initConds, 
		    float objFunOptimValue, 
		    int numberFcnEval, int returnCode);

  
  void SetParams(map<string, vector<string> > inputParams);
  void PrintInfo(ostream& outs, const string& ci = "");

  /*! This method gets the MPI ranks on each node, key are hostnames and values are vector of ranks in the node 
    \param [in, out] nodes gets and  sorts the ranks in each node
   */
  void GetRanksInNodes (map<string, vector<int> >& nodes);

  /*! In case of auto parallelism the method sets the appropriate parallelism to be used in the forward calcuation 
    \param [in, out] parallelism if the choice of parallelism is inappropriate it flag an error. 
   */
  void Parallelism (string& parallelism) ;

  /*! Decides the potential work that the calling process can do (worker, master, or redundant)
    \param [in] nodes map of sorted ranks per node
    \param [in] calling process rank
    \param [in] deviceIdOrNumThreads if the process can do work the cuda device id in case of cuda or the number of threads in case of omp 
    \param [in] parallelism cuda or omp
    \param [in] number of processes 
    \param [in] host_name calling process hostname
    \param [out] pt potential process task 
   */
  void GetMyWorkKind (map<string, vector<int> >& nodes, int rank,
		      int& deviceIdOrNumThreads, const string& parallelism, 
		      int nprocs, const string& host_name, ProcessTask& pt );

  /*! writes to log file or standard out the processes configurations */
  void PrintRanksInNodes (const map<string, vector<int> >& nodes, const string& prefix = "" ) const;


};

#endif
