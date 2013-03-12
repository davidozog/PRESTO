#include "env.hpp"
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
using namespace std;

string Env::getUserName() {
  register struct passwd *pw;
  register uid_t uid;

  uid = geteuid ();
  pw = getpwuid (uid);
  if (pw) {
    return string(pw->pw_name);
  }
  return string("");
}

string Env::getUid() {
  register struct passwd *pws;
  pws = getpwuid(geteuid());

  if (pws) {
    ostringstream stringStream;
    stringStream << pws->pw_uid;
    string uid = stringStream.str();
    return uid;
  }
  else return string("");
}
