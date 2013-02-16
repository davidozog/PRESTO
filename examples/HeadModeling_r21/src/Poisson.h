/*! \class Poisson
* \brief 3D Poisson equation solver abstract class
* \author Adnan Salman
* \author NeuroInformatic Center
* \date 2012
*/

#ifndef __POISSON__
#define __POISSON__

#include <stdlib.h>
#include <iomanip>
#include <iostream>

#include <map>
#include <vector>
#include <string>
#include <algorithm>

#include "HeadModel.h"
#include "grid_point.h"

using std::ostream;
using std::cout;
using std::map;
using std::vector;

/*! \todo 

   (1) Move mpGrid structure to PoissonAdi class only \n
   (2) Only keep geometry array, and final solution array in this class

*/

class Poisson {

 protected:

  int                      mNx; //!< Dimension of computational grid in x  
  int                      mNy; //!< Dimension of computational grid in y  
  int                      mNz; //!< Dimension of computational grid in z  

  int                      mNx1; //!< = mNx - 1
  int                      mNy1; //!< = mNy - 1
  int                      mNz1; //!< = mNz - 1

  int                      mNN;  //!< = Nx*Ny    
  int                      mN;   //!< = Nx*Ny*Nz 

  int                      mNmax;  //!< Largest dimension maximum(Nx, Ny, Nz) 
  int                      mKmax;  //!< Maximum number of iterations
    
  float                    mDx, mDy, mDz; //!< Mesh size in meters 
  float                    mHx, mHy, mHz; //!< Voxel size in meter 
  
  GridPoint               *mpGrid;        //!< mesh see GridPoint
  const HeadModel         *mHeadModel;

  //! Map contains the locations of the sensors, keys are sensors IDs and values are the Index in the flat array geometry 
  map<int, int>            mElectrodesLocations;

  //! Skull Normals -- contains skull voxel position (3) and normal (3 values)
  vector<vector<float> >   mNormals;

  float                    mTimeStep;  //!< TOBE Removed: time step for ADI only 
  float                    mTolerance; //!< Convergence tolerance 
  unsigned int             mNumberOfIterations; //!< Number of iterations untill convergence 
  double                   mExecutionTime;      //!< Excution time in seconds
    
  vector<float>            mTissuesConds;   //!< Tissue conductivities
  vector<std::string>      mTissuesNames;   //!< Tissue names
  vector<int>              mTissuesLabels;  //!< Tissue labels
    
  //! Current sources and sink, keys are location in the geometry flat array and values are the current  
  map<int, float>          mCurrentSources; 

  //! This is used to save the original tissue of the current source/sink if optionally decided to flip 
  //! it temporarly to a different tissues type. 
  map<int, float*>         mOriginalCurrentSourceTissue;

  bool                     mSourcesChanged; //!< flag the state of current source configuration 
  bool                     mCondsChanged;   //!< flag the state of conductivity values  
  bool                     mAnisotropicModel; //!< Handles both isotropic or anisotropic skull tissue 

  vector<float>            mBoneMineralDens;  //!< bone minerals density (attenuation or hounsfield units) 
  vector<float>            mBoneMineralConds; //!< bone conductivity corresponding to mineral density
  float                    mBmdCondFactor;    //!< proprtionality factor to compute conductivity from Bone density
  bool                     mBoneDensityMode;  //!< flag to set the computation mode 
  int                      mSkullIndex;       //!< skull tissue index

  //! Sets current source-sink given the index 
  int AddCurrentSourceIndex(int idx, float current, int source_tissue=-1);
  int AddCurrentSourceIndex(int idx, float current,
			      string source_tissue = ""); 
    
  void SetBoneMineralConds(float bmdCondFactor); //!< from bone mineral density compute conductivity

  //! Removes current source-sink given the index 
  int RemoveCurrentSourceIndex(int idx);

  //! Computes flat array index from sub
  inline int Index(int i, int j, int k) {return (i + j * mNx + k * mNN );};
  
  //!abstract class constructors are private
  Poisson();

public:

    //these methods are common for all forward solvers 
    virtual ~Poisson();
    virtual void init();
    virtual void reinit();
 
    /////////////////////////////////////////////////////////////////////////
    //// Conductivity relted methods 
    /////////////////////////////////////////////////////////////////////////

    vector<float>  GetTissueConds();
    float          GetTissueConds(const string& tissueName);
    float          GetTissueConds(const vector<int>& voxel);

    void           SetTissueConds(std::vector<float> tissuesConds); 
    void           SetTissueConds(int idx, float tisseCond);
    int            SetTissueConds(string tissueName, float tissueCond);
    void           SetTissueConds(std::vector<float> someTissueConds, 
				  vector<int> whichTissuesIndex);
    virtual void   SetConductivitiesAni(std::vector<float> someTissueConds, 
					vector<int> whichTissueIndex);   

    virtual int    SetTangentToNormalRatio(float tangToRadialRatio){};
    virtual int    SetSkullNormalCond(float skullRadialCond){};

    //! from set the skull model
    void SetBoneDensityMode(bool mode);


    
    /////////////////////////////////////////////////////////////////////////
    ////Sensors relted methods 
    /////////////////////////////////////////////////////////////////////////
    
    int            SetSensorsMap(char* sensorsFileName);
    map<int, int>  GetSensorsMap(){ return mElectrodesLocations;};
    int            GetTopSensorId();
    int            GetNumSensors() { return mElectrodesLocations.size(); }

    /////////////////////////////////////////////////////////////////////////
    //// geomtry Methodes .... setting and printing the geomtry       
    //// The geomtry is the distribution of the tissues labels on the grid 
    //// cells                                                        
    /////////////////////////////////////////////////////////////////////////
    
    int SetGeometry(char* geometryFileName);
    int SetGeometryAscii(char* geometryFileName);
    int SetGeometryBin(char* geometryFileName);
    int SetHeadModel(const HeadModel& headModel);

    int GetNumTissues() {return mTissuesNames.size();}    
    vector<std::string>  GetTissueNames(); 
    vector<int>          GetTissueLabels(); //not used
    
    ////////////////////////////////////////////////////////////////////////
    //// Sources related methods 
    ////////////////////////////////////////////////////////////////////////
    
    int AddCurrentSource(int i, int j, int k, float current, 
			 int sourceTissue = -1);
    int AddCurrentSource(int i, int j, int k, float current, 
			 string sourceTissue);

    int AddCurrentSource(int *x, float current, int sourceTissue = -1);
    int AddCurrentSource(int *x, float current, string sourceTissue);

    int AddCurrentSource(int sensorId, float current, int sourceTissue = -1);
    int AddCurrentSource(int sensorId, float current, string sourceTissue);

    int AddCurrentDipole(vector<int> x, float current, int sourceTissue=-1);
    int AddCurrentDipole(vector<int> x, float current, string sourceTissue);

    int RemoveCurrentSource(int i, int j, int k);
    int RemoveCurrentSource(int* x);
    int RemoveCurrentSource(int sensorId);
    int RemoveCurrentSource(std::string kind = "all");

    void UpdateCurrentValues(float current);
    std::vector<int> GetCurrentSources();
    
    ////////////////////////////////////////////////////////////////////////
    //// Solution related methods 
    ////////////////////////////////////////////////////////////////////////

    virtual int     Solve(string& compParallelism, int cudaDeviceOrOmpThreads)=0;

    map<int, float> SensorsPotentialMap();
    vector<float>   SensorsPotentialVec();
    vector<float>   VolumePotentials();
    float           VoxelPotential(const vector<int>& pos);
    vector<float>   VoxelElectricField(const vector<int>& pos);
    vector<float>   VoxelCurrentField( const vector<int>& pos);
    vector<float>   VoxelElectricField(int x, int y, int z);

    void            WriteSolution(const string& filen, char mode);
    virtual void    PrintInfo(ostream& out = cout);
    double          GetExecutionTime() { return mExecutionTime; }
    
    ////////////////////////////////////////////////////////////////////////
    //// Solvers related methods 
    ////////////////////////////////////////////////////////////////////////

    void            SetMaxIterations(int kmax) { mKmax = kmax;}
    int             GetMaxIterations() { return mKmax; }
    unsigned int    GetNumIterations() { return mNumberOfIterations; }

    void            SetConvTolerance(float tolerance) { mTolerance = tolerance;}
    float           GetConvTolerance() { return mTolerance; }

    virtual void    SetConvEps(float eps) {};
    virtual float   GetConvEps() {};

    virtual void    SetConvCheck(int convergenceCheck){};
    virtual float   GetConvCheck(){};

    virtual void    SetTimeStep(float timeStep) { mTimeStep = timeStep;}
    virtual float   GetTimeStep() { return mTimeStep; }
};

#endif
