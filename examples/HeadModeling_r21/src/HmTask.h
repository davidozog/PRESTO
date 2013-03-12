#ifndef __HM_TASK__
#define __HM_TASK__

#include <string>
#include <vector>
#include <map>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>

using std::string ;
using std::vector ;
using std::map;


class HmTask {
  public:
    map<string, vector<string> > params;
    map<int, float> potentials;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version){
      ar & params & potentials;
    }
};


#endif
