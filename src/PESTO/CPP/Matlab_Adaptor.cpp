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

}

void Matlab_Adaptor::Launch_CE(char *flags){

  FILE *fp;
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

  //fp = popen(mPath, "r");
  int i;
  if (system(NULL)) puts ("Ok");
    else exit (EXIT_FAILURE);
  printf ("Executing command Matlab...\n");
  i=system (mPath);
  printf ("The value returned was: %d.\n",i);

  //while ( fgets( line, sizeof line, i)) {
  //  //printf ("%s", line);
  //  cout << line << flush;
  //}

  //pclose (fp);

}

char * Matlab_Adaptor::get_name(){
  return name;
}

char * Matlab_Adaptor::set_name(){
  name = (char *)"matlab";
}

