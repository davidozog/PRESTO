#include <fstream>
#include <sstream>

#include <algorithm>
#include <cmath>
#include <set>
#include <iomanip>

#include "HeadModel.h"
#include "HmUtil.h"

using std::cout;
using std::endl;
using std::cerr;
using std::ios;
using std::ofstream;
using std::ifstream;
using std::stringstream;
using std::setw;

HeadModel::HeadModel(map<string, vector<string> >& hmInputParams){
  Init(hmInputParams);
}

void HeadModel::Init(map<string, vector<string> >& hmInputParams){

  string         param;
  vector<string> paramValue, inTissueNames, inTissueConds;

  string  dataPath              = "./";
  string  brainkDataDir         = "";
  string  dataSetKind           = "indiv_odip_CT";

  string  dipolesFileName       = "";
  string  dipolesOrientFileName = "";
  string  geometryFileName      = "";
  string  sensorsFileName       = "";

  map<string, vector<string> >::const_iterator iter;

  for (iter = hmInputParams.begin(); iter != hmInputParams.end(); iter++){
  
    param = iter->first; paramValue = iter->second;

    if (param == "datapath") {
      if (!paramValue[0].empty()) dataPath = paramValue[0];
      HmUtil::TrimStrSpaces(dataPath);
      if (dataPath[dataPath.length()-1] != '/') dataPath += "/";

      if (dataPath[0] != '/'){
	dataPath = string(getenv ("HEAD_MODELING_HOME"))+"/"+ dataPath;
      }
    }

    else if (param == "braink_data_dir") brainkDataDir = paramValue[0];
    else if (param == "data_set_kind") dataSetKind = paramValue[0];

    else if (param == "geometry") geometryFileName = paramValue[0];
    else if (param == "sensors")  sensorsFileName = paramValue[0];
    else if (param == "dipoles_file_name") dipolesFileName = paramValue[0];
    else if (param == "dipoles_orient_fname") dipolesOrientFileName = paramValue[0];

    else if (param == "tissues") inTissueNames = hmInputParams["tissues"];
    else if (param == "tissues_conds") inTissueConds = hmInputParams["tissues_conds"];
  
  }

  if (brainkDataDir.empty()) {

    if (geometryFileName.empty()) 
      HmUtil::ExitWithError( "IOError: no geometry data file specified ... " );
    
    if (sensorsFileName.empty())
      HmUtil::ExitWithError( "IOError: no sensors data file specified ...");

    if (dipolesFileName.empty())
      HmUtil::ExitWithError("IOError: No dipoles data file specified ... ");

    dipolesFileName = dataPath + dipolesFileName;
    geometryFileName = dataPath + geometryFileName;
    sensorsFileName = dataPath + sensorsFileName;

    int err = init_fdm(geometryFileName, sensorsFileName, dipolesFileName, "", "");
    if (err) HmUtil::ExitWithError("IOError: loading brainK data ... ");

  }

  else{

    string braink_data_path = dataPath + brainkDataDir; 
    int err = Init(braink_data_path, dataSetKind, "");
    if (err) HmUtil::ExitWithError("IOError: loading brainK data ... " + braink_data_path);
  }
}


HeadModel::HeadModel(const string& braink_dir, const string& data_set_kind) {

  Init(braink_dir, data_set_kind);

}

inline int HeadModel::Index(int i, int j, int k){

  return ( i + j * mGeometryShape[0] + k * mGeometryShape[0] * mGeometryShape[1] );

}

int HeadModel::Init(const string& rBrainkDataPath, const string& rDataSetKind, 
		    const string& rDataSetPrefix ){

  string geometryFileName = "";
  string sensorsFileName  = "";
  string dipolesFileName1 = "";
  string dipolesFileName2 = "";
  string normalsFileName  = "";
  string bmdFileName      = "";
  mTissuesCfgFileName     = "";

  GetBkFileNames(  rBrainkDataPath,   rDataSetKind,     rDataSetPrefix, 
		   geometryFileName,  sensorsFileName,  dipolesFileName1, 
		   dipolesFileName2,  normalsFileName,  bmdFileName, mTissuesCfgFileName );

  if (mTissuesCfgFileName.empty()){
    mTissuesCfgFileName = getenv ("HEAD_MODELING_HOME");
    mTissuesCfgFileName += "/cfg/tissue_config.txt";
  }

  LoadTissueLookUpTable();

  LoadGeometryBk(geometryFileName);
  LoadSensorsBk(sensorsFileName);

  mDipolesOrientationMap.clear();
  mDipolesPosMap.clear();

  if ( !dipolesFileName1.empty() ) LoadDipolesBk( dipolesFileName1 );
  if ( !dipolesFileName2.empty() ) LoadDipolesBk( dipolesFileName2 );
  if ( !normalsFileName.empty()  ) LoadNormals( normalsFileName );
  if ( !bmdFileName.empty() )      LoadBmdData( bmdFileName );

  AdjustPaddingAir(4);  //todo: use global PADDING_AIR_PLANES = 4
  CheckSensorsPositions();
  CheckDipolesPositions();

  if ( !rDataSetPrefix.empty() ) PrintDataSet( rDataSetPrefix );

  return 0;
}

int HeadModel::GetBkFileNames(const string& rBrainkDataPath, 
			      const string& rDataSetKind, 
			      const string& rDataSetPrefix, 
			      string& rGeometryFileName, 
			      string& rSensorsFileName, 
			      string& rDipolesFileName1, 
			      string& rDipolesFileName2, 
			      string& rNormalsFileName,
			      string& rBmdFileName, 
			      string& tissuesCfgFileName ){

  vector<string> files = vector<string>();

  int err = HmUtil::GetFilesInDirectory(rBrainkDataPath, files);
  if (err) HmUtil::ExitWithError("IOError: getting braink directory ");
  
  map<string, map<string, string> > bkFileExt;
  BkFilesExtensions(bkFileExt); //load file exten from cfg file

  if (bkFileExt.find(rDataSetKind) == bkFileExt.end()) 
    HmUtil::ExitWithError("DataError: unrecognized subject data type ");
 
  string geometryFileExtens =  bkFileExt[rDataSetKind]["geom_ext"];
  string sensorsFileExtens  =  bkFileExt[rDataSetKind]["sns_ext"];
  string dipolesFileExten1  =  bkFileExt[rDataSetKind]["dip1_ext"];
  string dipolesFileExten2  =  bkFileExt[rDataSetKind]["dip2_ext"];
  string normalsFileExtens  =  bkFileExt[rDataSetKind]["cran_norm_ext"];
  string bmdFileExtens      =  bkFileExt[rDataSetKind]["bmd_ext"];
  string tissueCfgExtens    =  bkFileExt[rDataSetKind]["tissue_ext"];

  rGeometryFileName  = "";
  rSensorsFileName   = "";
  rDipolesFileName1  = "";
  rDipolesFileName2  = "";
  rNormalsFileName   = "";
  rBmdFileName       = "";
  tissuesCfgFileName = "";

  int pos, fsize;

  for (unsigned int i=0; i<files.size(); i++){

    fsize = files[i].size();

    if ( (pos=fsize-geometryFileExtens.size())>0 && 
	 files[i].substr(pos)==geometryFileExtens )

      // geometry file
      if (!rGeometryFileName.empty()){
	HmUtil::ExitWithError("IOError: Ambiguous two geometry files exist \n" + 
			rGeometryFileName + "\n" + 
			rBrainkDataPath + "/" + files[i]);
      }
      else 
	rGeometryFileName = rBrainkDataPath + "/" + files[i];

    // sensors file 
    else if ( (pos =  fsize - sensorsFileExtens.size()) > 0 && 
	      files[i].substr(pos) == sensorsFileExtens )

      if (!rSensorsFileName.empty()){
	HmUtil::ExitWithError("IOError: Ambiguous two sensor files exist\n" + 
			rSensorsFileName + "\n" + 
			rBrainkDataPath + "/" + files[i]);
      }
      else 
	rSensorsFileName = rBrainkDataPath +"/"+ files[i];
    

    // dipoles 1 file
    else if ( (pos=fsize-dipolesFileExten1.size())> 0 && 
	      files[i].substr(pos)==dipolesFileExten1 )
      if (!rDipolesFileName1.empty()){
	HmUtil::ExitWithError( "IOError: ambiguous two dipole1 files exist " +
			 rDipolesFileName1 + "\t" + rBrainkDataPath + "/" + 
			 files[i] );
      }
      else 
	rDipolesFileName1 = rBrainkDataPath + "/" + files[i];


    // dipoles 2 file
    else if ( !dipolesFileExten2.empty() && 
	      (pos=fsize-dipolesFileExten2.size())> 0
	      &&files[i].substr(pos)== dipolesFileExten2 )

      if (!rDipolesFileName2.empty()){
	HmUtil::ExitWithError( "IOError: ambiguous two dipoles2 files exist " + 
			  rDipolesFileName2 + "\t" + rBrainkDataPath    + 
			  "/" + files[i] );
      }
      else 
	rDipolesFileName2 = rBrainkDataPath + "/" + files[i];


    // normals file
    else if ( !normalsFileExtens.empty() && 
	      (pos=fsize-normalsFileExtens.size()) > 0 &&
	      files[i].substr(pos)== normalsFileExtens )

      if (!rNormalsFileName.empty()){
	HmUtil::ExitWithError("IOError: ambiguous two normals files " + rNormalsFileName 
			+ "\t" + rBrainkDataPath + "/" + files[i] );
      }
      else 
	rNormalsFileName = rBrainkDataPath + "/" + files[i];

    // bone mineral density file
    else if ( !bmdFileExtens.empty() && 
	      (pos=fsize-bmdFileExtens.size()) > 0 &&
	      files[i].substr(pos)== bmdFileExtens )

      if (!rBmdFileName.empty()){
	HmUtil::ExitWithError("IOError: ambiguous two BMD files " + rBmdFileName
			+ "\t" + rBrainkDataPath + "/" + files[i] );
      }
      else 
	rBmdFileName = rBrainkDataPath + "/" + files[i];

    // tissue configuration file
    else if ( !tissueCfgExtens.empty() && 
	      (pos=fsize-tissueCfgExtens.size()) > 0 &&
	      files[i].substr(pos)== tissueCfgExtens )

      if (!tissuesCfgFileName.empty()){
	HmUtil::ExitWithError("IOError: ambiguous two tissue cfg files:\n" + tissuesCfgFileName
			+ "\n" + rBrainkDataPath + "/" + files[i] + "\n");
      }
      else 
	tissuesCfgFileName = rBrainkDataPath + "/" + files[i];

  }

  if (rGeometryFileName.empty()) 
    HmUtil::ExitWithError("IOError: no geometry file specified ");

  if (rSensorsFileName.empty())  
    HmUtil::ExitWithError("IOError: no sensors file exist ");

}

int HeadModel::BkFilesExtensions(map<string, map<string, string> >& dataSetType){				  
  
  string brainkFileExtensions = getenv ("HEAD_MODELING_HOME");
  brainkFileExtensions += "/cfg/braink_file_ext.cfg";

  ifstream fe(&brainkFileExtensions[0]);
  if (!fe) 
    HmUtil::ExitWithError("Can't open tissue configuration file" + brainkFileExtensions);
  
  string line, dataSetKind, fileKind, fileEtension;

  while(getline(fe, line, '\n')){
    HmUtil::TrimStrSpaces(line);    
    if (line[0] == '#' || line.empty()) continue;
    stringstream lines(stringstream::in | stringstream::out );
    lines << line;
    lines >>  dataSetKind >> fileKind >> fileEtension;

    HmUtil::TrimStrSpaces(dataSetKind);
    HmUtil::TrimStrSpaces(fileKind);
    HmUtil::TrimStrSpaces(fileEtension);
    
    dataSetType[dataSetKind][fileKind] = fileEtension;
  }

  /*
  map<string, map<string, string> >::iterator it =  dataSetType.begin();
  for (; it !=  dataSetType.end(); it++){
    map<string, string> mm = it->second;
    for (map<string, string>::iterator itr = mm.begin(); itr != mm.end(); itr++){
      cout << it->first << "  " << itr->first << "  " << itr->second << endl;
    }
  }
  */
}

int HeadModel::LoadTissueLookUpTable(){

  // string tissueConfigPath;
  // tissueConfigPath = getenv ("HEAD_MODELING_HOME");
  // tissueConfigPath += "/cfg/tissue_config.txt";

  ifstream ts(&mTissuesCfgFileName[0]);
  if (!ts) 
    HmUtil::ExitWithError("Can't open tissue configuration file" + mTissuesCfgFileName);
  
  string line, tname, flags;
  float  tcond;
  int    tlabel;

  mTissueLookUpName.clear();
  mTissueLookUpLabel.clear();
  mTissueLookUpCond.clear();
  mTissueAllowSensorsLu.clear();
  mTissueAllowDipolesLu.clear();

  while(getline(ts, line, '\n')){

    HmUtil::TrimStrSpaces(line);    

    if (line[0] == '#' || line.empty()) continue;
    stringstream lines(stringstream::in | stringstream::out );

    lines << line;
    lines >> tname >> tlabel >> tcond >> flags;

    HmUtil::TrimStrSpaces(tname);
    HmUtil::TrimStrSpaces(flags);

    mTissueLookUpName.push_back(tname);
    mTissueLookUpLabel.push_back(tlabel);
    mTissueLookUpCond.push_back(tcond);

    if (flags.size() != 2 ) {
      HmUtil::ExitWithError("Error: parsing tissue properties file: " + flags);
    }
    for (int i=0; i<2; i++){
      if (flags[i] != '0' && flags[i] != '1'){
	HmUtil::ExitWithError("Error: parsing tissue properties file less: " + flags);
      }
    }
    bool b1 = (flags[0] == '1');
    bool b2 = (flags[1] == '1');
    mTissueAllowSensorsLu.push_back(b1);
    mTissueAllowDipolesLu.push_back(b2);
  }

  
  //No two tissue with same label is allowed

  for (unsigned int i=0; i<mTissueLookUpLabel.size(); i++){
    for (unsigned int j=i+1; j<mTissueLookUpLabel.size(); j++){

      if (mTissueLookUpLabel[i] == mTissueLookUpLabel[j]){
	HmUtil::ExitWithError("Duplicate tissue labels in tissue_config file " + 
			HmUtil::IntToString(mTissueLookUpLabel[i]));
      }
    }
  }

    
  //  for (int i=0; i<mTissueLookUpName.size(); i++){
  //    cout << setw(10) << mTissueLookUpName[i] << setw(10) << mTissueLookUpLabel[i] << setw(10)
  //	 << mTissueLookUpCond[i] << setw(10) << (int) mTissueAllowSensorsLu[i] << setw(10)
  //    	 << (int) mTissueAllowDipolesLu[i] << endl;
  //  }

  return 0;
}

int HeadModel::LoadGeometryBk(const string& geometry_fname){

  std::ifstream geomf(&geometry_fname[0], std::ios::in | std::ios::binary);
   
  if (!geomf.is_open()) 
    HmUtil::ExitWithError("IOError: Cannot open geometry file: " + geometry_fname);
    
  // skip 3 int index
  geomf.seekg(3*sizeof(int ), ios::beg);

  // read size
  mGeometryShape.resize(3);

  geomf.read((char*) &mGeometryShape[0], 3*sizeof( unsigned int ));

  // skip 3 float origin
  geomf.seekg(3*sizeof(float), ios::cur);

  // get 3 floats spacing
  mVoxelSizeMm.resize(3);
  geomf.read((char*) &mVoxelSizeMm[0], 3*sizeof(float));

  // size
  int N = mGeometryShape[0]*mGeometryShape[1]*mGeometryShape[2];
  mGeometry.resize(N);
  geomf.read((char*) &mGeometry[0], N*sizeof(unsigned char));

  geomf.close();
  
  SetTissuesProp();

}

int HeadModel::SetTissuesProp(){
  
  // find tissue labels in geometry file
  std::set<int> s(mGeometry.begin(), mGeometry.end());

  // Check corresponding tissues are defined 
  for (std::set<int>::const_iterator it = s.begin(); it != s.end(); it++){
    
    bool recognizedTissue = (std::find(mTissueLookUpLabel.begin(), 
				       mTissueLookUpLabel.end(), *it) 
			     != mTissueLookUpLabel.end());
    
    if (!recognizedTissue)
      HmUtil::ExitWithError("Unrecognized tissue lebel: " + HmUtil::IntToString(*it) );
  }


  // translate tissue labels to be from 0 to numOfTissues 
  vector<int>   translateLabel;
  map<int, int> mapLabels;

  for (int i=0; i<mTissueLookUpLabel.size(); i++){

    bool inTissue = (std::find(s.begin(), s.end(), mTissueLookUpLabel[i]) 
		     != s.end());
    if (inTissue){
      translateLabel.push_back(i);
    }
  }

  for (int i=0; i<translateLabel.size(); i++){
    mTissueNames.push_back(mTissueLookUpName[translateLabel[i]]);
    mTissueConds.push_back(mTissueLookUpCond[translateLabel[i]]);
    mTissueAllowSensors.push_back(mTissueAllowSensorsLu[translateLabel[i]]);
    mTissueAllowDipoles.push_back( mTissueAllowDipolesLu[translateLabel[i]]);
    mapLabels[mTissueLookUpLabel[translateLabel[i]]] = i;
    mTissueLabels.push_back(i);

  }

  vector<unsigned char> reLabeledGeometry(mGeometry.size());
  for (int i=0; i<mGeometry.size(); i++){
    reLabeledGeometry[i] = mapLabels[mGeometry[i]];
  }

  mGeometry = reLabeledGeometry;

  return 0;
}




const vector<unsigned char>& HeadModel::GetGeometry() const { 

  return mGeometry;

}

const map<int, vector<int> >& HeadModel::GetSensorsMap() const { 

  return mSensorsMap;

}

const map<string, vector<int> >& HeadModel::GetReferenceSensorsMap() const{ 

  return mRefSensorsMap;

}

const map<int, vector<int> >& HeadModel::GetDipolesPosMap() const{ 

  return mDipolesPosMap;

}

const map<int, vector<float> >& HeadModel::GetDipolesOrientsMap() const{ 
  return mDipolesOrientationMap;
}

const vector<string>& HeadModel::GetTissuesNames() const { 

  return mTissueNames; 

}

vector<float>& HeadModel::GetTissuesConds() { 

  return mTissueConds; 

}

const vector<float>& HeadModel::GetGeometryResolution() const { 

  return mVoxelSizeMm; 

}

const vector<vector<float> >& HeadModel::GetNormals() const {

  return mCraniumNormals;

}

int HeadModel::init_fdm(const string& geometryFileName, 
			const string& sensorsFileName, 
			const string& dipolesFileName1,
			const string& dipolesFileName2, 
			const string& normalsFileName,
			const string& dataSetPrefix){

  if (geometryFileName.empty())
    HmUtil::ExitWithError( "IOError: no geometry file specified " );
  
  if (sensorsFileName.empty())
    HmUtil::ExitWithError( "IOError: no sensors file exist " );

  int err = LoadGeometryFdm(geometryFileName);
  if (err ) return err;

  err = LoadSensorsFdm(sensorsFileName);
  if (err ) return err;

  mDipolesOrientationMap.clear();
  mDipolesPosMap.clear();
  mCraniumNormals.clear();

  if (!dipolesFileName1.empty()) 
    LoadDipolesFdm(dipolesFileName1);
  
  if (!dipolesFileName2.empty())
    LoadDipolesFdm(dipolesFileName2);

  if (!normalsFileName.empty())
    LoadNormals(normalsFileName);

  CheckSensorsPositions();
  CheckDipolesPositions();
  
  if (!dataSetPrefix.empty()) PrintDataSet(dataSetPrefix);

  return 0;
}


int HeadModel::CheckDipolesPositions(){

  int Nx = mGeometryShape[0];  
  int Ny = mGeometryShape[1] ;  
  int Nz = mGeometryShape[2];

  for (map<int, vector<int> >::iterator it = mDipolesPosMap.begin(); 
       it != mDipolesPosMap.end(); it++){

    if (it->second[0] < 1 || it->second[0] >= Nx-1 ||
	it->second[1] < 1 || it->second[1] >= Ny-1 ||
	it->second[2] < 1 || it->second[2] >= Nz-1 ){

      HmUtil::ExitWithError("DataError: one or more dipole position is off image  ");

    }

    int inTissue = (int) mGeometry[Index(it->second[0], it->second[1], it->second[2])];
    if ( !mTissueAllowDipoles[inTissue] )
      HmUtil::ExitWithError("DataError: one or more dipoles are in illegal tissue: " + 
		      mTissueNames[inTissue] );
  }
}

int HeadModel::CheckSensorsPositions(){
  
  int Nx = mGeometryShape[0];  
  int Ny = mGeometryShape[1] ;  
  int Nz = mGeometryShape[2];

  vector<int>::iterator itr;
  for (map<int, vector<int> >::iterator it = mSensorsMap.begin(); it != mSensorsMap.end(); it++){
    if (it->second[0] < 0 || it->second[0] > Nx-1 ||
	it->second[1] < 0 || it->second[1] > Ny-1 ||
	it->second[2] < 0 || it->second[2] > Nz-1 ){
      HmUtil::ExitWithError("DataError: one or more sensor position is off image  ");
    }
  }
}

void  HeadModel::PrintDataSet(const string& data_set_prefix){

  if (data_set_prefix.empty())
    return;
  
  string geomf = data_set_prefix + "_geom.txt";
  string snsf  = data_set_prefix + "_sns.txt";
  string rsnsf = data_set_prefix + "_rsns.txt";
  string dposf = data_set_prefix + "_srcs_pos.txt";
  string dorif = data_set_prefix + "_srcs_ori.txt";
  string geomh = data_set_prefix + "_geom.hdr";

  ofstream outs(&geomf[0]);
  for (int i=0; i<mGeometry.size(); i++){
    outs << (int) mGeometry[i] << endl;
  }
  outs.close();

  outs.open(&snsf[0]);
  for (map<int, vector<int> >::iterator it = mSensorsMap.begin(); 
       it != mSensorsMap.end(); it++){
    outs << it->first << "  " << it->second[0] << "  " << it->second[1]  
	 << "  " << it->second[2] << endl;
  }
  outs.close();

  outs.open(&dposf[0]);
  for (map<int, vector<int> >::iterator it = mDipolesPosMap.begin(); 
       it != mDipolesPosMap.end(); it++){
    outs << it->first << "  " << it->second[0] << "  " << it->second[1]  
	 << "  " << it->second[2] << endl;
  }
  outs.close();

  outs.open(&dorif[0]);
  for (map<int, vector<float> >::iterator it = mDipolesOrientationMap.begin(); 
       it != mDipolesOrientationMap.end(); it++){
    outs << it->first << "  " << it->second[0] << "  " << it->second[1]  
	 << "  " << it->second[2] << endl;
  }
  outs.close();

  outs.open(&geomh[0]);
  outs << "Dimension = " <<  mGeometryShape[0] << "  " << mGeometryShape[1] 
       << "  " << mGeometryShape[2] << endl;
  outs << "Resolution = " << mVoxelSizeMm[0]   << "  " <<mVoxelSizeMm [1]   
       << "  " << mVoxelSizeMm[2]   << endl;

  outs << "Tissue names = ";
  for (int i =0; i<mTissueNames.size(); i++){
    outs << mTissueNames[i] << "  ";
  }
  outs << endl;

  outs << "Tissue labels = ";
  for (int i =0; i<mTissueLabels.size(); i++){
    outs << mTissueLabels[i] << "  ";
  }
  outs << endl;

  outs << "Tissue conds = ";
  for (int i =0; i<mTissueConds.size(); i++){
    outs << mTissueConds[i] << "  ";
  }
  outs << endl;

  for (map<string, vector<int> >::iterator it = mRefSensorsMap.begin();
       it != mRefSensorsMap.end(); it++){
    outs << it->first << "  " << it->second[0] << "  " << it->second[1] 
	 << "  " << it->second[2] << endl;
  }  
}

void HeadModel::AdjustPaddingAir(int amount){

  int Nx = mGeometryShape[0];  
  int Ny = mGeometryShape[1] ;  
  int Nz = mGeometryShape[2];
  int N  = Nx*Ny*Nz;      
  int NN = Nx*Ny;

  vector<int> air = GetPaddingAir();

  vector<int> add_air_planes(6, 0); //number of planes to add or remove in each direction
  for (int i=0; i<air.size(); i++) add_air_planes[i] = amount-air[i];

  /// new geometry is constructed such that the head is surounded by
  /// amount planes of air in each side

  int Mx, My, Mz;
  Mx = Nx + add_air_planes[0] + add_air_planes[1];
  My = Ny + add_air_planes[2] + add_air_planes[3];
  Mz = Nz + add_air_planes[4] + add_air_planes[5];

  //new addition
  int addMoreZ = 0, addMoreY = 0, addMoreX = 0;
  if (Mz%4 != 0) addMoreZ = (4 - Mz%4);
  if (My%4 != 0) addMoreY = (4 - My%4);
  if (Mx%4 != 0) addMoreX = (4 - Mx%4);

  Mz += addMoreZ;
  My += addMoreY;
  Mx += addMoreX;

  int MM = Mx * My;
  int M  = MM * Mz;

  vector<unsigned char> adjust_geometry(M, 0);

  int i, j, k, ijk;
  for (k=amount; k < Mz-amount-addMoreZ; k++){
    for (j=amount; j < My-amount-addMoreY; j++){
      for (i=amount; i < Mx-amount-addMoreX; i++){

	ijk = i + j * Mx + k * MM;
	int idx = Index(i-add_air_planes[0], j-add_air_planes[2], 
			k-add_air_planes[4]);
	adjust_geometry[ijk] = mGeometry.at(idx);

      }
    }
  }

  mGeometryShape[0] = Mx;
  mGeometryShape[1] = My;
  mGeometryShape[2] = Mz;
  mGeometry = adjust_geometry;

  // adjust sensors 
  if (!mSensorsMap.empty()){
    for (map<int, vector<int> >::iterator it = mSensorsMap.begin();
	 it != mSensorsMap.end(); it++){
      it->second[0] += add_air_planes[0];
      it->second[1] += add_air_planes[2];
      it->second[2] += add_air_planes[4];
    }
  }

  // adjust reference sensors 
  if (!mRefSensorsMap.empty()){
    for (map<string, vector<int> >::iterator it = mRefSensorsMap.begin();
	 it != mRefSensorsMap.end(); it++){
      it->second[0] += add_air_planes[0];
      it->second[1] += add_air_planes[2];
      it->second[2] += add_air_planes[4];
    }
  }

  // adjust dipoles 
  if (!mDipolesPosMap.empty()){
    for (map<int, vector<int> >::iterator it = mDipolesPosMap.begin();
	 it != mDipolesPosMap.end(); it++){
      it->second[0] += add_air_planes[0];
      it->second[1] += add_air_planes[2];
      it->second[2] += add_air_planes[4];
    }
  }

  // adjust normals
  if (!mCraniumNormals.empty()){
    for (int i=0; i<mCraniumNormals.size(); i++){
      mCraniumNormals[i][0] += add_air_planes[0];
      mCraniumNormals[i][1] += add_air_planes[2];
      mCraniumNormals[i][2] += add_air_planes[4];
    }
  }
  
  // adjust bone mineral densit 
  if (!mBmdMap.empty()){
    map<int, float> bmdMap;
    for (map<int, float >::iterator it = mBmdMap.begin();
	 it != mBmdMap.end(); it++){
      int ind = it->first; 
      int z = ind/NN;
      int y = (ind%NN)/Nx; 
      int x = ((ind%NN)%Nx);
      
      x += add_air_planes[0];
      y += add_air_planes[2];
      z += add_air_planes[4];
      
      int ind2 = x + y * Mx + z * MM;
      bmdMap[ind2] = it->second;
    }
    mBmdMap = bmdMap;

  }      


  

 
}


int HeadModel::LoadNormals(const string& rNormalsFileName){
 
  int Nx = mGeometryShape[0];  int Ny = mGeometryShape[1] ;  int Nz = mGeometryShape[2];
  int N  = Nx*Ny*Nz;      int NN = Nx*Ny;

  std::ifstream Fp(&rNormalsFileName[0]);
  if (!Fp) HmUtil::ExitWithError( "Set_normals Tan: cannot open input file: "+ rNormalsFileName );
  int numAnisotropic;
  Fp >> numAnisotropic;
  mCraniumNormals.resize(numAnisotropic, vector<float>(6));
  int x, y, z;
  float dirx, diry, dirz;

  vector<string>::const_iterator it = find(mTissueNames.begin(), mTissueNames.end(), "skull");
  if (it == mTissueNames.end()){
    HmUtil::ExitWithError("No skull tissue available ");
  }

  int skullLabel = mTissueLabels[it-mTissueNames.begin()];
  float ftol = .0001;

  for(int idx=0; idx<numAnisotropic; idx++) {
    Fp >> mCraniumNormals[idx][0] >> mCraniumNormals[idx][1] >> 
      mCraniumNormals[idx][2] >> mCraniumNormals[idx][3] >> 
      mCraniumNormals[idx][4] >> mCraniumNormals[idx][5];
    
    if (mCraniumNormals[idx][0] < 0 || mCraniumNormals[idx][0] > Nx || 
	mCraniumNormals[idx][1] < 0 || mCraniumNormals[idx][1] > Ny || 
	mCraniumNormals[idx][0] < 0 || mCraniumNormals[idx][0] > Nz){

      HmUtil::ExitWithError("Normals for outbound voxel ");

    }

    //    if (mGeometry[iindex(mCraniumNormals[idx][0], mCraniumNormals[idx][1], 
    //			 mCraniumNormals[idx][2]) ] != skullLabel){
    if (mGeometry[Index(mCraniumNormals[idx][0], mCraniumNormals[idx][1], 
			mCraniumNormals[idx][2]) ] != skullLabel){
      HmUtil::ExitWithError("normal for nonskull voxel");
    }
    if (abs(1-sqrt(mCraniumNormals[idx][3]*mCraniumNormals[idx][3] + 
		   mCraniumNormals[idx][4]*mCraniumNormals[idx][4] + 
		   mCraniumNormals[idx][5]*mCraniumNormals[idx][5])) > ftol){
      HmUtil::ExitWithError("Skull normal is not normalized ");
    }
  }
}

int HeadModel::LoadBmdData(const string& bmdFileName){

  int Nx = mGeometryShape[0];  int Ny = mGeometryShape[1] ;  int Nz = mGeometryShape[2];
  int N  = Nx*Ny*Nz;      int NN = Nx*Ny;

  std::ifstream Fp(&bmdFileName[0]);
  if (!Fp) HmUtil::ExitWithError( "LoadBmdData: cannot open input file: "+ bmdFileName );

  int numBmdVoxels;
  Fp >> numBmdVoxels;

  int   ind;
  float bmd;

  for (int i=0; i<numBmdVoxels; i++){
    Fp >> ind >> bmd;
    mBmdMap[ind] = bmd;
    //    cout << "BMD: " << ind << "  " << bmd << endl;
  }

  
  return 1;

}

int HeadModel::LoadGeometryFdm(const string& geometry_fname){

    int i, tissue, num_tissues;
    std::ifstream geomf(&geometry_fname[0], std::ios::in | std::ios::binary);
    
    if (!geomf.is_open()) 
      HmUtil::ExitWithError("IOError: cannot open geometry input file: " + geometry_fname);
    
    char* geom_id = new char[32];
    geomf.read(geom_id, 32*sizeof(char));

    // read size
    mGeometryShape.resize(3);
    geomf.read((char*) &mGeometryShape[0], 3*sizeof( unsigned int ));
    int N = mGeometryShape[0]*mGeometryShape[1]*mGeometryShape[2];
    geomf.read((char*) &num_tissues, sizeof(unsigned int));

    char * t = new char[32];
    for (i=0; i<num_tissues; i++) {
        geomf.read((char*) t, 32*sizeof(char));
        mTissueNames.push_back(t);
	mTissueNames[i] = mTissueNames[i].substr(0,31);
        HmUtil::TrimStrSpaces(mTissueNames[i]);
    }
    delete [] t;

    mTissueLabels.resize(mTissueNames.size());
    mTissueConds.resize(mTissueNames.size());
    map<string, int> tlabel_map;
    
    for (i=0; i<mTissueLabels.size(); i++){
      geomf.read((char*) &mTissueLabels[i], sizeof(unsigned char));
      tlabel_map[mTissueNames[i]] = mTissueLabels[i];
    }

    for (i=0; i<mTissueConds.size(); i++)
      geomf.read((char*) &mTissueConds[i], sizeof(float));

    mVoxelSizeMm.resize(3);
    geomf.read((char*) &mVoxelSizeMm[0], 3*sizeof(float));

    mGeometry.resize(N);
    geomf.read((char*) &mGeometry[0], N*sizeof(unsigned char));

    int x;
    vector<int> tissue_exist(mTissueNames.size(), 0);
 
    for (int i=0; i<N; i++){
      x = mGeometry[i];
      if (x < 0 || x>=mTissueNames.size()){
	HmUtil::ExitWithError("DataError: Unrecognized tissue label " + 
			HmUtil::IntToString( mGeometry[i] ) ); 
      }
      tissue_exist[x] = 1; 
    }

    for (int i=0; i<tissue_exist.size(); i++){
      if (!tissue_exist[i]){
	mTissueNames.erase(mTissueNames.begin()+i);
	mTissueConds.erase(mTissueConds.begin()+i);
	mTissueLabels.erase(mTissueLabels.begin()+i);
      }
    }

    mTissueAllowSensors.resize(mTissueNames.size());
    mTissueAllowDipoles.resize(mTissueNames.size(), true);


    for (unsigned int i=0; i<mTissueNames.size(); i++){
      mTissueAllowSensors[i] = false;
      if (mTissueNames[i] == "scalp")
	mTissueAllowSensors[i] = true;
    }

    geomf.close();
    return 0;
}
 
int HeadModel::LoadSensorsBk(const string& sensors_fname){

  std::ifstream geomf(&sensors_fname[0]);
  if (!geomf.is_open()) {
    std::cerr << "IOError: cannot open sensors file: " << sensors_fname << std::endl;
    return 1;
  }

  string sns_id;
  float dx = mVoxelSizeMm[0];
  float dy = mVoxelSizeMm[1];
  float dz = mVoxelSizeMm[2];
  float x, y, z;
  int sns_ids_int;

  while (geomf.good()){

    geomf >> sns_id >> x >> y >> z;
    if (sns_id == "FidNz" || sns_id == "FidT9" || sns_id == "FidT10" 
	|| sns_id == "Cz"){
      /* This breaks the Bone Mineral Density code with PL data - not sure why yet */
      mRefSensorsMap[sns_id].push_back((int) x/dx);
      mRefSensorsMap[sns_id].push_back((int) y/dy);
      mRefSensorsMap[sns_id].push_back((int) z/dz);
      //mRefSensorsMap[sns_id].push_back((int) x);
      //mRefSensorsMap[sns_id].push_back((int) y);
      //mRefSensorsMap[sns_id].push_back((int) z);
    }
    else if (sns_id[0] == 'E'){
      sns_ids_int = atoi(&sns_id[1]);
      /* This breaks the Bone Mineral Density code with PL data - not sure why yet */
      mSensorsMap[sns_ids_int].push_back((int) x/dx);
      mSensorsMap[sns_ids_int].push_back((int) y/dy);
      mSensorsMap[sns_ids_int].push_back((int) z/dz);
      //mSensorsMap[sns_ids_int].push_back((int) x);
      //mSensorsMap[sns_ids_int].push_back((int) y);
      //mSensorsMap[sns_ids_int].push_back((int) z);
    }
    else {
      cerr << "DataError: unrecognized sensor id " << sns_id << endl;
      return 1;
    }
  }
  geomf.close();
  return 0;
}


int HeadModel::LoadSensorsFdm(const string& sensors_fname){

  std::ifstream geomf(&sensors_fname[0]);
  if (!geomf.is_open()) {
    std::cerr << "IOError: cannot open sensors file: " << sensors_fname << std::endl;
    return 1;
  }

  string sns_id;
  float dx = mVoxelSizeMm[0];
  float dy = mVoxelSizeMm[1];
  float dz = mVoxelSizeMm[2];
  float x, y, z;
  int sns_ids_int;

  while (geomf.good()){

    geomf >> sns_id >> x >> y >> z;
    if (sns_id == "FidNz" || sns_id == "FidT9" || sns_id == "FidT10" 
	|| sns_id == "Cz"){
      mRefSensorsMap[sns_id].push_back((int) x/dx);
      mRefSensorsMap[sns_id].push_back((int) y/dy);
      mRefSensorsMap[sns_id].push_back((int) z/dz);
    }
    else if (sns_id[0] == 'E' || HmUtil::IsNumber(sns_id)){
      if (sns_id[0] == 'E')
	sns_ids_int = atoi(&sns_id[1]);
      else
	sns_ids_int = atoi(&sns_id[0]);

      mSensorsMap[sns_ids_int].push_back((int) x/dx);
      mSensorsMap[sns_ids_int].push_back((int) y/dy);
      mSensorsMap[sns_ids_int].push_back((int) z/dz);
    }
    else {
      cerr << "DataError: unrecognized sensor id " << sns_id << endl;
      return 1;
    }
  }
  geomf.close();
  return 0;
}


int HeadModel::LoadDipolesBk(const string& dip_fname){
  std::ifstream dips;

  if (!dip_fname.empty()){
    dips.open(&dip_fname[0], ios::in | ios::binary | ios::ate);

    if (!dips.is_open()) {
      std::cerr << "IOError: cannot open dipoles file: " << dip_fname << std::endl;
      return 1;
    }
  }

  std::ifstream::pos_type fsize;
  fsize = dips.tellg();
  dips.seekg(0, ios::beg);

  int ndipoles;
  dips.read((char*) &ndipoles, sizeof( int ));

  int * dip_loc = new int[3*ndipoles];
  dips.read((char*) dip_loc,3* ndipoles*sizeof( int ));

  // In case of more than one dipoles file
  // The dipoles are appended to the end of mDipolesPosMap
  int did = mDipolesPosMap.size();
  int i = did + 1;

  for (int k=0; i<=ndipoles+did; i++, k+=3){

    mDipolesPosMap[i].push_back(dip_loc[k]);
    mDipolesPosMap[i].push_back(dip_loc[k+1]);
    mDipolesPosMap[i].push_back(dip_loc[k+2]);

  }

  delete [] dip_loc;

  //comput how much left
  long left = fsize - dips.tellg();

  // Check if we have more data for dipoles orientations 
  if (left < 6*ndipoles*sizeof(float) + 2*sizeof(int)){
    dips.close();
    return 0;
  }

  //skip ndipoles float dipoles locations and num of floats
  dips.seekg(sizeof(int ), ios::cur);
  dips.seekg(3*ndipoles*sizeof(float ), ios::cur);

  //read oriented dipoles orientation
  int num_odipoles;
  dips.read((char*) &num_odipoles, sizeof( int ));

  //cout << "num_odipoles = " << num_odipoles << endl;

  float * dip_dir = new float[3*num_odipoles];
  dips.read((char*) dip_dir,3* num_odipoles*sizeof( float ));
  did = mDipolesOrientationMap.size();
  i = did + 1;

  for (int k=0; i<=num_odipoles+did; i++, k+=3){

    mDipolesOrientationMap[i].push_back(dip_dir[k]);
    mDipolesOrientationMap[i].push_back(dip_dir[k+1]);
    mDipolesOrientationMap[i].push_back(dip_dir[k+2]);
  }

  delete [] dip_dir;
  dips.close();
  return 0;
}


int HeadModel::LoadDipolesFdm(const string& dip_fname){
  std::ifstream dips;

  if (!dip_fname.empty()){
    dips.open(&dip_fname[0]);

    if (!dips.is_open()) {
      std::cerr << "IOError: cannot open dipoles file: " << dip_fname << std::endl;
      return 1;
    }
  }

  int num_dipoles;
  int id, x, y, z;
  dips >> num_dipoles;

  for (int k=0; k<num_dipoles; k++){
    dips >> id >> x >> y >> z;

    mDipolesPosMap[id].push_back(x);
    mDipolesPosMap[id].push_back(y);
    mDipolesPosMap[id].push_back(z);
  }

  dips.close();
  return 0;
}

vector<int> HeadModel::GetPaddingAir(){


  vector<int> air(6, 0);
  bool iter = true;

  int Nx = mGeometryShape[0];
  int Ny = mGeometryShape[1];
  int Nz = mGeometryShape[2];
  int NN = Nx*Ny;

  for (int k=0; k<Nz && iter; k++){
    for(int j=0; j<Ny && iter; j++){
      for(int i=0; i<Nx && iter; i++){
	if ((int) mGeometry[Index(i,j,k)] != 0){
	  air[4] = k;
	  iter = false;
	}}}}

  iter = true;
  for (int k=Nz-1; k>0 && iter; k--){
    for(int j=0; j<Ny && iter; j++){
      for(int i=0; i<Nx && iter; i++){
	if ((int) mGeometry[Index(i,j,k)] != 0){
	  air[5] = Nz-k-1;
	  iter = false;
	}}}}

  iter = true;
  for (int j=0; j<Ny && iter; j++){
    for(int k=0; k<Nz && iter; k++){
      for(int i=0; i<Nx && iter; i++){
	if ((int) mGeometry[Index(i,j,k)] != 0){
	  air[2] = j;
	  iter = false;
	}}}}

  iter = true;
  for (int j=Ny-1; j>0 && iter; j--){
    for(int k=0; k<Nz && iter; k++){
      for(int i=0; i<Nx && iter; i++){
	if ((int) mGeometry[Index(i,j,k)] != 0){
	  air[3] = Ny-j-1;
	  iter = false;
	}}}}

  iter = true;
  for (int i=0; i<Nx && iter; i++){
    for(int k=0; k<Nz && iter; k++){
      for(int j=0; j<Ny && iter; j++){
	if (mGeometry[Index(i,j,k)] != 0){
	  air[0] = i;
	  iter = false;
	}}}}

  iter = true;
  for (int i=Nx-1; i>0 && iter; i--){
    for(int k=0; k<Nz && iter; k++){
      for(int j=0; j<Nx && iter; j++){
	//	if (mGeometry[iindex(i,j,k)] != 0){
	if (mGeometry[Index(i,j,k)] != 0){
	  air[1] = Nx-i-1;
	  iter = false;
	}}}}
  
  return air;
}

vector<float>  HeadModel::GetVoxelSizeMm() const {
  return mVoxelSizeMm;
}


void HeadModel::PrintInfo(ostream& outs) {

  /*
  if (!braink_data_dir.empty()){
    outs << setw(30) << "Braink data set: " << braink_data_dir << endl;
    outs << setw(30) << "Data set kind: "   << data_set_kind << endl;
  }
  outs << setw(30) << "FDM geometry file: " << mGeometryFileName << endl;
  outs << setw(30) << "FDM sensors file: "  <<  mSensorsFileName << endl;

  if (!mDipolesFileName1.empty()) {
    outs << setw(30) << "FDM dipoles positions file 1: " <<  mDipolesFileName1 << endl;
  }

  if (!mDipolesFileName1.empty()) 
    outs << setw(30) << "FDM dipoles orientation file 2: " <<  mDipolesFileName2 << endl;
  }

  if (!mNormalsFileName.empty()){
    outs << setw(30) << "Normals file name: " <<  mNormalsFileName << endl;
  }
  */
}

