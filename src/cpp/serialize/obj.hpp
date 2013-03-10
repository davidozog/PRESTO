#pragma once
// File obj.hpp

// Forward declaration of class boost::serialization::access
namespace boost {
namespace serialization {
class access;
}
}

class Obj {
public:
  // Serialization expects the object to have a default constructor
  Obj() : d1_(-1), d2_(false) {}
  Obj(int d1, bool d2) : d1_(d1), d2_(d2) {}
  bool operator==(const Obj& o) const {
    return d1_ == o.d1_ && d2_ == o.d2_;
  }
private:
  int  d1_;
  bool d2_;

  // Allow serialization to access non-public data members.
  friend class boost::serialization::access;

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version) {
    ar & d1_ & d2_;  // Simply serialize the data members of Obj
  }
};
