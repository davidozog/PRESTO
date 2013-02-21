#include "CE_Adaptor.h"
#include "Matlab_Adaptor.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;


Matlab_Adaptor::Matlab_Adaptor(): CE_Adaptor(){
  name = (char *)malloc(7);
  name = (char *)"matlab";
}


Matlab_Adaptor::~Matlab_Adaptor(){

  // delete instance;
  free(name);

}

void Matlab_Adaptor::Launch_CE(const char *flags, int background){

  FILE *fp;
  int i;
  char line[130]; 

  char *mPath;
  char *sPath;
 // char *mEnv = "source ~/.zshrc; ";
 // strcpy(mPath, mEnv);
 // strcat(mPath, getenv("MATLAB"));
  mPath = getenv ("MATLAB");

  if (mPath!=NULL) {
    strcat (mPath, flags);
    printf ("MATLAB path is: %s\n",mPath);
  }
  else {
    printf ("MATLAB env var is not defined\n");
    exit (-1);
  }

  setenv("STINGRAY","/home/dave/School/Stingray/trunk",1);

  if ( background ) {
    fp = popen(mPath, "r");
    while ( fgets( line, sizeof line, fp)) {
      //printf ("%s", line);
      fflush(fp);
    }
    pclose (fp);
  }
  else {
    if (system(NULL)) puts ("Ok");
      else exit (EXIT_FAILURE);
    printf ("Executing command Matlab...\n");
    i = system(mPath);
    printf ("The value returned was: %d.\n",i);
  }

}

char * Matlab_Adaptor::get_name(){
  return name;
}

char * Matlab_Adaptor::set_name(){
  name = (char *)"matlab";
}

