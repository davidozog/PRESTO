/*! \class LeadField
* \brief Lead Field Matrix generation 
* \author Adnan Salman
* \author NeuroInformatic Center
* \date 2012
*/

#ifndef __LFM_GENER__
#define __LFM_GENER__

#include <iostream>
#include <stdlib.h>
#include <fstream>

#include <algorithm>
#include <cmath>

#include <mpi.h>

#include "Poisson.h"

/*!
  \enum ProcessTask 
  \brief Defines the MPI processors tasks   
  It is possible to have redundant process which the code ignores 
*/
enum ProcessTask { 
  MASTER,  /*!< The process is master.   */ 
  WORKER,  /*!< The process is a worker. */ 
  NOTHING  /*!< The process is redundant and shouldn't consume any resources. */ 
};

/*! \todo  
  (1) Have the code prints tissue labels as specified before translation \n                                
  (2) Consider making a class to parse the input parameters file and return objects                             
*/

class LeadField{

private:

  string                   mDataPath;            //!< Input data set path
  string                   mOutputNamePrefix;    //!< Used to generate output files 

  float                    mCurrent;             //!< Not used to be removed 
  float                    mDipoleMomentMagAm;   //!< In Ampere.Meter  

  int                      mRefElectrodeId;      //!< Which electrode to use as reference 
  vector<int>              mRefElectrodePos;     //!< Its 3D voxel coordinate

  bool                     mUseTriples;          //!< Compute triples first and then oriented  
  bool                     mUseReciprocity;      //!< Use reciprocity principle

  int                      mRank;                //!< MPI process rank
  HeadModel                mHeadModel;           //!< Head model object
  map<int, vector<float> > mDipolesMap;          //!< Keys are dipoles Ids and values are dipoles positions
  int                      mMaster;              //!< Master process rank

  string                   mParallelism;         //!< OMP or CUDA
  ProcessTask              mMyTask;              //!< MASTER, WORKER, or NOTHING
  int                      mDviceOrThreads;      //!< Cuda device id or OMP number threads 
  bool                     mSwapBytes;           //!< If requested endianness is different from native
  string                   mUnitStandard;        //!< LFM output units (Volts or nVolts)
  string                   mLfmFileNameTrip;     //!< Triples LFM file name
  string                   mLfmFileNameOri;      //!< Oriented LFM file name 

  Poisson * P;                                   //!< Poisson equation solver

  int  SetDipoleMoments();        //!< Generates mDipolesMap of oriented dipoles given HeadModel object
  int  SetDipolesTriples();       //!< Generates mDipolesMap of triples dipoles given HeadModel object

  void MasterReciprocity();       //!< Do Master work when using reciprocity 
  void MasterForward();           //!< Do Master work when compute dipoles lead field

  //! Workers task when using reciprocity -- computes the potentail at a sensor corresponding to all dipoles
  void WorkerReciprocity(const string& mode, int threads); 

  //! Workers task when not using reciprocity -- computes the potentail all sensors corresponding to a single dipole
  void WorkerForward(const string& mode, int threads);

  //! Computes and writes oriented LFM given triples LFM -- also does postprocessing if requested  
  void WriteOrientedLfm();

  //! Post process a LFM -- switch endianness and/or change units 
  void LfmPostProcess(const string& lfmFileName);

  //! Updates the progress time in the log file
  inline  void UpdateProgress(int sensor, int recievedFrom,  int total, 
			      double stime, const string& mode);

  //! Master calls this function to get a list of worker that are ready for computation 
  vector<int>  GetWorkersRanks();

  //! Writes the computed potentials at a sensor corresponds to all dipoles 
  void WriteSensorField(ostream& outs, int sensor, 
			int num_dipoles, vector<float>& rdata);

  //! Writes the computed potentials at all sensors corresponds to a dipole
  void WriteDipoleField(ostream& outs, int dipole, 
			int numDipoles, vector<float>& rdata);

  //! Sets the reference electrode
  int SetReferenceSensor(string& ref_electrode);

  //! Determine each MPI process task (master, worker, or nothing)
  void MyTask(string& parallelism, int& deviceIdOrNumThreads, 
	      ProcessTask& myTask);

  //! Prints the LFM header file (.hdr)
  int  PrintHeader();


public:
  
  /*! \brief default constructor 
   */
  LeadField();

  /*! \brief Constructor given parameters from the head model input file 
    \param params a map where the keys are hm input file commands and 
                  the values are vector of strings  

  */
  LeadField(map<string, vector<string> > params);

  /*! \brief Initializes the LeadField object from the head model input file 
    \param params a map where the keys are hm input file commands and 

  */
  int Init(map<string, vector<string> > params);

  /*! \brief Default initialization of the LeadField object  */
  int Init();

  /*! \brief Computes the LFM
  */ 
  void Compute();

};

#endif
