#ifndef __VAI__VOXEL__
#define __VAI__VOXEL__
#include <cmath>
using namespace std;

class VaiVoxel {

 public:
  float Sxx;
  float Syy;
  float Szz;

  float Sxy;
  float Sxz;
  float Syz;

  float Syx;
  float Szx;
  float Szy;
  float source;
  bool isanis;
  
  float dirx;
  float diry;
  float dirz;

  float elemIdx;
  float inElemIdx;


  VaiVoxel(){
    Sxx = Syy = Szz = Sxy = Sxz = Syz = Syx = Szx = Szy = source = 0.0;
    dirx = diry = dirz = 0;
    isanis = false;
    elemIdx = -1;
    inElemIdx = -1;

  }

  VaiVoxel(const VaiVoxel& vox){
     Sxx = vox.Sxx;
     Syy = vox.Syy ;
     Szz = vox.Szz;
    
     Sxy = vox.Sxy;
     Sxz = vox.Sxz;
     Syz = vox.Syz;

     Syx = vox.Syx;
     Szx = vox.Szx;
     Szy = vox.Szy;
     source = vox.source;
     isanis = vox.isanis;

     dirx  = vox.dirx;
     diry  = vox.diry;
     dirz  = vox.dirz;

     elemIdx = vox.elemIdx;
     inElemIdx = vox.inElemIdx;
     
  }

  VaiVoxel(float _Sxx, float _Syy, float _Szz, float _Sxy, float _Sxz, float _Syz, float _source, bool _isanis){
    Sxx = _Sxx;
    Syy = _Syy;
    Szz = _Szz;

    Sxy = _Sxy;
    Sxz = _Sxz;
    Syx = _Sxy;

    Syx = _Sxy;
    Szx = _Sxz;
    Szy = _Syz;
    source = _source;
    isanis = _isanis;
  }

  void PrintInfo(){
    cout << Sxx << "  "  << Syy << "  "<< Szz << "  " << Sxy << "  " << Sxz << "  " 
	 << Syx << "  " << Szx << "  " << Syz << "  " << Szy << "  " << isanis << std::endl; 

  }

  bool IsNan(){
    return (isnan(Sxx) || isnan(Syy) || isnan(Szz) || isnan(Sxy) || isnan(Sxz) || isnan(Syx) 
	    || isnan(Szx) || isnan(Syz) || isnan(Szy));
     
  }

  bool IsInf(){
    return (isinf(Sxx) || isinf(Syy) || isinf(Szz) || isinf(Sxy) || isinf(Sxz) || isinf(Syx) 
	    || isinf(Szx) || isinf(Syz) || isinf(Szy));
     
  }

};


#endif

