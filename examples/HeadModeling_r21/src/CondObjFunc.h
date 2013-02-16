/*! \class Poisson
* \brief 3D Poisson equation solver abstract class
* \author Adnan Salman
* \author NeuroInformatic Center
* \date 2012
*/

#ifndef __COND_OBJ_FUNC__
#define __COND_OBJ_FUNC__

#include "Poisson.h"
#include <vector>
#include "ObjFunc.h"
#include "current_inj_dat.h"

using std::vector;

class CondObjFunc : public ObjFunc{

 private:

  /*! Poisson Equation solver */
  Poisson       *mP;

  /*! Vector contains the indices of the variable tissues */
  vector<int>   mVarParamIndices; //paramInd;  

  /*! Vector of measured potentials */
  vector<float> mV; 

  /*! Vector of excluded electrodes */
  vector<int>   mExecludedElecIds; //execlude_elec;

  /*! Cartesian position of the reference electrode */
  vector<int>   mRefElectrodePos; //ref_electrode_pos;

  /*! Process rank used for debugging only */
  int           mRank;
  
  /*! Use cuda or omp */
  string        mParallelism; //;comp_mode;

  /*! Cuda device ID or OMP number threads */
  int           mOmpThreadsOrDevice;

  public:
  
  CondObjFunc();
  virtual ~CondObjFunc();

  void init(Poisson * P,  vector<int> varTissueIndices, string objFunMethods, 
	    vector<int> refElecPosition, int rank,  int ompThreadsOrDevice, 
	    string parallelism);

  /*! 
  \brief Calculates the l2-norm objective function (root mean square error).
  \param Pot vector of computed potentials  
  \f[ E = \bigg( \frac{1}{N_{tot}} \sum_{i=1}^N (\phi^p_i - V_i)^2 \bigg) ^{1/2}, \f]
  where \f$ E \f$ is the l2-norm, \f$ N_{tot} \f$ is the total number of electrodes (minus
  the number of excluded electrodes), \f$ \phi^p_i \f$ is the \f$ i^{th} \f$ computed 
  potential (\f$ \texttt{Pot[i]} \f$), and \f$ V_i \f$ is the \f$ i^{th} \f$ measured
  potential.                                                                  */
  float l2norm(vector<float> Pot);

  /*!
  \brief Calculates the relative root mean square error
  \param Pot vector of computed potentials 
  \f[ relE = \sqrt{ \frac{ \sum_{i=1}^N (\phi_i - V_i)^2 } { \sum_{i=1}^{N} \phi_i^2} }, \f]
  where \f$ relE \f$ is the relative norm, \f$ N \f$ is the total number of electrodes (minus
  the number of excluded electrodes), \f$ \phi_i \f$ is the \f$ i^{th} \f$ computed 
  potential (\f$ \texttt{Pot[i]} \f$), and \f$ V_i \f$ is the \f$ i^{th} \f$ measured
  potential.                                                                  */
  float relL2norm(vector<float> Pot);

  /*! 
  \brief Calculates the normalized l2-norm objective function (root mean square error).
  \param Pot vector of computed potentials  
  \f[ normE = 100 * \sum_{i=1}^N \frac{ (\phi_i - V_i)^2 }{ V_i^2 }, \f]
  where \f$ normE \f$ is the normalized l2-norm, \f$ \phi_i \f$ is the \f$ i^{th} \f$ computed 
  potential (\f$ \texttt{Pot[i]} \f$), and \f$ V_i \f$ is the \f$ i^{th} \f$ measured
  potential.                                                                  */
  float normL2norm(vector<float> Pot);


  float rdm(vector<float> Pot);

  /*! 
    \brief Calculates the optimization objective function.
    \param N  number of variables  
    \param variables vector of N variables 
    \param fvalue objective function value 
  */
  virtual int operator()(int N, vector<float> variables, float& fvalue);

  /*! 
    \brief Pointer to objective function to use
  */
  float (CondObjFunc::*mObjFuncMethod)(vector<float>);

  /*! 
    \brief sets the EIT data 
  */
  void set_measured_data(const CurrentInjecData& cid);

};

#endif
