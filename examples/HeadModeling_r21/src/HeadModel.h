#ifndef __SRC_HEAD_MODEL__
#define __SRC_HEAD_MODEL__

#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <iostream>
#include <fstream>
#include <iomanip>

#include <string>
#include <map>
#include <vector>
#include <algorithm>

using std::vector;
using std::string;
using std::map;
using std::ostream;

/* TODO

   (1) remove friendship 
   (2) print info to a header file
       - geometry file name 
       - sensors 
       - etc
*/



class HeadModel{
  friend class Poisson;
  friend class LeadField;

private:

  vector<unsigned char>      mGeometry;
  vector<int >               mGeometryShape;
  vector<float>              mVoxelSizeMm;
  vector<string>             mTissueNames;
  vector<int>                mTissueLabels;
  vector<float>              mTissueConds;

  map<int, vector<int> >     mSensorsMap;
  map<string, vector<int> >  mRefSensorsMap;
  map<int, vector<int> >     mDipolesPosMap;
  map<int, vector<float> >   mDipolesOrientationMap;
  vector<vector<float> >     mCraniumNormals;
  map<int, float>            mBmdMap;

  vector<string>             mTissueLookUpName;
  vector<int>                mTissueLookUpLabel;
  vector<float>              mTissueLookUpCond;
  vector<string>             mTissueLookUpFlags;
  vector<bool>               mTissueAllowSensorsLu;
  vector<bool>               mTissueAllowDipolesLu;


  vector<bool>               mTissueAllowSensors;
  vector<bool>               mTissueAllowDipoles;
  vector<bool>               mTissueLabelsBk;
  string                     mTissuesCfgFileName;

  /*! \brief returns number of air planes padding the head in each side */
  vector<int> GetPaddingAir(); 


  /*! \brief adjust the number of padding air planes around the head 
    \param [in] numberOfPlanes about the final number of planes in each side
                while maintaining the size of multiple of 2
  */
  void  AdjustPaddingAir(int numberOfPlanes);


  //  int   GetFilesInDirectory(const string& directory, vector<string> &files);

  /*! \brief uploads geometry from BrainK image data file */
  int   LoadGeometryBk(const string& geometryFileName);

  /*! \brief uploads sensors locations from BrainK sensors file */
  int   LoadSensorsBk (const string& sensorsFileName);

  /*! \brief uploads dipoles locations from BrainK dipoles file */
  int   LoadDipolesBk (const string& dipolesFileName);

  /*! \brief uploads geometry from old geometry file format */
  int   LoadGeometryFdm(const string& geometry_fname);

  /*! \brief uploads sensors locations from old sensors file format */
  int   LoadSensorsFdm (const string& sensors_fname);

  /*! \brief uploads dipoles locations from old dipoles file format */
  int   LoadDipolesFdm (const string& dip_fname);

  /*! \brief uploads anisotropic voxels normals locations and orientations 
    from Braink or old dipoles file format. */
  int   LoadNormals(const string& rNormalsFileName);

 /*! \brief uploads bone mineral density data  */
  int   LoadBmdData(const string& bmdFileName);

  /*! \brief upload tissue properties definitions from configuration file 
   \attention not used yet */
  int   LoadTissueLookUpTable();

  /*! \brief list all files in the specified braink directory */
  int   GetBkFileNames( const string& rBrainkDataPath, const string& rDataSetKind, 
			const string& rDataSetPrefix, string& rGeometryFileName, 
			string& rSensorsFileName, string& rDipolesFileName1, 
			string& rDipolesFileName2, string& rNormalsFileName,
			string& rBmdFileName, string& tissuesFileName );
  
  /*! \brief loads the required braink file extensions   */
  int BkFilesExtensions(map<string, map<string, string> >& dataSetType);


  /*! \brief  defines a lookup tissue properties   */
  //  int   LoadRecognizedTissues();

  /*! \brief  check if sensors are located in the allowed tissues  */
  int   CheckSensorsPositions();

  /*! \brief  check if dipoles are located in the allowed tissues  */
  int   CheckDipolesPositions();

  /*! \brief using tissue tags in geometry it extracts the properties of the corresponding tissue  */
  int   SetTissuesProp();
  
  /*! \brief return flat array index given sub indices  */
  inline int Index(int i, int j, int k);

public:

  /*! \brief Constructor */
  HeadModel(){};
  HeadModel(const string& brainkDir, const string& dataSetKind);

  /*! \brief loads data and builds head model
    \param brainkDir braink data set
    \param dataSetKind path to braink data set
    \param dataSetPrefix if set the HeadModel will be output using naming based
    on dataSetPrefix
   */

  /*! \brief loads the data specified in the head modeling input file
    \param hmInputParams head modeling input parameters map
   */
  HeadModel(map<string, vector<string> >& hmInputParams);
  void Init(map<string, vector<string> >& hmInputParams);


  int Init(const string& brainkDir, 
	   const string& dataSetKind, 
	   const string& dataSetPrefix = "" );

  /*! \brief loads all data from old format and builds HeadModel */
  int init_fdm(const string& geometryFileName,  
	       const string& sensorsFileName, 
	       const string& dipolesFileName1, 
	       const string& dipolesFileName2, 
	       const string& normalsFileName, 
	       const string& dataSetPrefix = "");
  
  /*! \brief Outputs the head model data after modification (adjustments) */
  void PrintDataSet(const string& dataSet);

  /*! \brief returns the geometry  */
  const vector<unsigned char>&       GetGeometry() const;

  /*! \brief returns the sensors map  */
  const map<int, vector<int> >&      GetSensorsMap() const;

  /*! \brief returns reference sensors map  */
  const map<string, vector<int> >&   GetReferenceSensorsMap() const;

  /*! \brief returns dipoles locations   */
  const map<int, vector<int> >&      GetDipolesPosMap() const;

  /*! \brief return dipoles orientations   */
  const map<int, vector<float> >&    GetDipolesOrientsMap() const;

  /*! \brief returns tissue names  */
  const vector<string>&              GetTissuesNames() const;

  /*! \brief returns tissue conductivities  */
  vector<float>&                     GetTissuesConds();

  /*! \brief returns voxel spacing in mm  */
  const vector<float>&               GetGeometryResolution() const;

  /*! \brief returns normals  */
  const vector<vector<float> >&      GetNormals() const;

  vector<float>              GetVoxelSizeMm() const;

  void PrintInfo(ostream& outs);

  bool AvailableSensors(){return (mSensorsMap.size() > 0);}

};

#endif
