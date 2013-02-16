#include "paral_sa.h"
#include <fstream>


using namespace std;

int SimAnneal::GenerateTrialPoint(int H, int N, vector<float> X, vector<float>  & x_try, float & F_try){
  
  x_try = X;
  x_try[H] = X[H] + RandUniform(-1,1) * mVm[H];

  //If x_try is out of bounds, select a point in bounds for the trial.
  if((x_try[H] < mVarLowerBound[H]) || (x_try[H] > mVarUpperBound[H])){
    x_try[H] = mVarLowerBound[H] + (mVarUpperBound[H] - mVarLowerBound[H])*RandUniform01();
    mNumberOutOfBoundMoves++;
  }

  //  int fnum_iter = (*mpObjFunc)(N, &x_try[0], F_try);
  int fnum_iter = (*mpObjFunc)(N, x_try, F_try);
  mNumberFcnEval++;
  return fnum_iter;
}


void SimAnneal::AdjustStepSize(vector<int> nacp, int N){

  // adjust the search redius length such that the number of accepted 
  // moves equals the number of rejected moves 
  // for more details see:
  // Corana, A., Marhesi, M., Martini, C. and Ridella, S., 
  // Minimizing Multimodal Functions of
  // Continuous Variables with the Simulated Annealing Algorithm, 
  // ACM Trans. on Mathematical
  // Software, 13(3), 1987
  // or my short review in the documentation section 

  for(int i = 0; i< N; i++){
    float R =( float)(nacp[i]) /(float)mNs;
    if (R > mUratio) 
      mVm[i] = mVm[i]*(1. + mCstep[i]*(R - mUratio)/mLratio);
    else if (R < mLratio) 
      mVm[i] = mVm[i]/(1. + mCstep[i]*((mLratio - R)/mLratio));
    
    if (mVm[i] > (mVarUpperBound[i]-mVarLowerBound[i])) 
      mVm[i] = mVarUpperBound[i] - mVarLowerBound[i];
  }
}


int SimAnneal::Optimize(vector<float> &X, float& F_optimal,  ObjFunc *func_ , ostream* outs){
  
  int rank;
  MPI_Comm_rank( mMpiComm, &rank );
  srand( 52434+rank*3445 );   //fix the random seed 

  mpObjFunc = func_;
  
  //function value at X                                    
  float F;   

  //to count the number of rejected moves
  int  num_rej_up;      

  //to count the number of new optimal points found
  int num_new_optimal;                   

  //number of accepted moves in direction I 
  vector<int> num_accepted_direction(mNumberOfVariables, 0);

  //trial point 
  vector<float> x_try(mNumberOfVariables);           

  //function value at the trial points  
  float  F_try;                     

  //best point so far
  vector<float> x_optimal(mNumberOfVariables);

  // holds the values of the functions at the end
  // of temprature mChperiod interval.   
  vector<float> F_check(mCheck, 1.0E20);
                                          
  int check_count = -1;
  mNumberAcceptedMoves = 0;
  mNumberOutOfBoundMoves = 0;
  mNumberFcnEval = 0;
  
  x_optimal = X;

  if (mTemp <= 0.0){
    cerr << " Initial temprature is not positive ."  << endl;
    return  -1;
  }

  //If the initial value is out of bounds, return with error error -2
  for( int i = 0; i<mNumberOfVariables; i++){
    if ((X[i] > mVarUpperBound[i]) || (X[i] < mVarLowerBound[i])) return -2;
  }

  //Evaluate the mpObjFunction at X.
  (*mpObjFunc)(mNumberOfVariables,X,F);   mNumberFcnEval++;  F_optimal = F;

  // Start the loop. It terminates if the algorithm succesfully optimizes the 
  // function or there are too many function evaluations (more than MAXEVL).

  float F_best;
  vector<float> X_best(mNumberOfVariables);

  int num_processors;;
  MPI_Comm_size( mMpiComm, &num_processors );
  int num_tasks = mNs/num_processors;
  mMaxFcnEval =  mMaxFcnEval/num_processors;

  MPI_Comm_rank( mMpiComm, &rank );

  float * Fp;
  float * Xp;

  if (rank == 0){
    Fp = new float[num_processors];
    Xp = new float[mNumberOfVariables*num_processors];
  }

  int numfcneval = 0;
  vector<int> numaccepteddirection(mNumberOfVariables);
  int terminate = 0;
  int pterminate = 0;

  if (outs)
    *outs << left;

  //  cout << left; 

  while(1){
    mNumberUpMoves = 0;   num_rej_up = 0;
    num_new_optimal = 0;   mNumberDownMoves = 0;
      
    //at the end of this loop temprature reschedualed
    for(int j = 0; j<mNt; j++){ 
      X_best = X;
      F_best = F;
		
      // at the end of this loop, maximum step length adjusted
      for(int k = 0; k< num_tasks; k++){          
	// consider all N direction when trying to find a trial  
	for(int l = 0; l<mNumberOfVariables; l++){                 
	  // Generate a trial point and evaluate the function 
	  // at this trial point.
	  int fnum_iter = GenerateTrialPoint(l,mNumberOfVariables, X, x_try, F_try);

	  if(mNumberFcnEval > mMaxFcnEval){
	    //	    cout << "terminate: max_fcn_eval" << endl; 
	    pterminate = 1;
	  }

	  if (outs){
	    *outs  << left <<  setw(5) << rank << setw(5) << fnum_iter << setw(5) << mNumberFcnEval 
		   << setw(13) << F_try << setw(13) << F_optimal;
	  }

	  if (outs){
	    for (int ii=0; ii<mNumberOfVariables; ii++) *outs << setw(13) << x_try[ii];
	    *outs << " : ";
	    for (int ii=0; ii<mNumberOfVariables; ii++) *outs << setw(13) << x_optimal[ii];
	    *outs << endl;
	  }

	  //Accept the new point if the function value decreases.
	  if(F_try <= F){
	    X = x_try;
	    F = F_try;  mNumberAcceptedMoves++;   num_accepted_direction[l]++;   
	    mNumberUpMoves++;
	    
	    //If less than any other point, record as new optimum.
	    if (F_try < F_best) { 
	      X_best = x_try;
	      F_best = F_try;
	    }
	    
	    if (F_try <= mTolerance){
	      //	      cout << "terminate: tol" << endl; 
	      pterminate = 2;
	    }
	  }
	  
	  else{
	    float p = exp((F - F_try)/mTemp);
	    float  rand = RandUniform01();
	    if (rand < p) {
	      X = x_try;
	      F = F_try; mNumberAcceptedMoves++;  num_accepted_direction[l]++;  
	      mNumberDownMoves++;
	    } else num_rej_up++;
	  }

	  if (terminate){
	    break;
	  }
	}

	if (terminate){
	  break;
	}
      }
      
      MPI_Allreduce(&pterminate,&terminate,1,MPI_INT, MPI_LOR, mMpiComm);
      MPI_Gather(&F_best, 1, MPI_FLOAT, Fp, 1, MPI_FLOAT, 0, mMpiComm);
      MPI_Gather(&X_best[0], mNumberOfVariables, MPI_FLOAT,Xp, mNumberOfVariables, MPI_FLOAT,0,mMpiComm);
      
      if (rank == 0){
	for (int kk=1; kk<num_processors; kk++){
	  if ( Fp[kk] < F_best){ 
	    F_best = Fp[kk];
	    for (int mm=0; mm<mNumberOfVariables; mm++) X_best[mm] = Xp[kk*mNumberOfVariables+mm];
	  }
	}	
	
	//If less than any other point, record as new optimum.
	if (F_best < F_optimal) { 
	  for(int i = 0; i< mNumberOfVariables; i++) x_optimal[i] = X_best[i];
	  F_optimal = F_best;
	  num_new_optimal++;
	}
      }

      //Adjust mVm so that approximately half of the evaluations are accepted.
      MPI_Reduce(&mNumberFcnEval,&numfcneval, 1, MPI_INT, MPI_SUM, 
		 0, mMpiComm);
      if (terminate) break;

      MPI_Allreduce(&num_accepted_direction[0], &numaccepteddirection[0], 
		    mNumberOfVariables, MPI_INT, MPI_SUM, mMpiComm);
	
      AdjustStepSize(numaccepteddirection, mNumberOfVariables);
      for(int i = 0; i< mNumberOfVariables; i++) num_accepted_direction[i] = 0;
    }

    if (!terminate && rank == 0){
      check_count++;
      if (check_count == mCheck) check_count = 0;

      //Check termination conditions.
      //bool terminate = false;
      F_check[check_count] = F_best;
      if ((F_optimal - F_best) <= mSimAnnEps) terminate = 3;
      for(int i = 0; i< mCheck; i++)
	if (abs(F_best - F_check[i]) > mSimAnnEps) terminate = 0;
      //      if (terminate)
      //	cout << "terminate: mSimAnnEps" << endl; 
    }
    
    MPI_Bcast(&terminate, 1, MPI_INT, 0, mMpiComm);

    F = F_optimal;
    for(int i = 0; i< mNumberOfVariables; i++) X[i] = x_optimal[i];

    //Terminate SA if appropriate.
    if (terminate){
      mNumberFcnEval = numfcneval;
      if (rank == 0){
	if (terminate == 1){
	  cout << "Terminate = max func evaluation exceeded" << endl;
	}
	else if (terminate == 2){
	  cout << "Terminate = tolerance value reached" << endl;
	}
	else if (terminate == 3){
	  cout << "Terminate = simulated annealing epsilon is reached " << endl;
	}
      }
      return terminate;
    }

    //if not terminate prepare for another loop
    MPI_Bcast(&x_optimal[0], mNumberOfVariables, MPI_FLOAT, 0, mMpiComm);
    MPI_Bcast(&F_optimal, 1, MPI_FLOAT, 0, mMpiComm);

    //prepare for another loop.
    mTemp = mRt*mTemp;
  }

  return 0;
}


SimAnneal::SimAnneal(int numberOfVariables, ObjFunc * pObjFunc, int rank){

  mNumberOfVariables = numberOfVariables;
  mpObjFunc = pObjFunc;
  mMpiComm = MPI_COMM_WORLD;

  mTolerance = .05;
  mRt = .8;//
  mSimAnnEps = 1.0E-4;//
  
  mNs = 12;//
  mNt = 3;//
  mCheck = 2;//
  mMaxFcnEval = 20000;//
    
  mUratio = .6;
  mLratio = .4;
  mTemp = 5.0;

  mVm.resize(mNumberOfVariables);;//
  mCstep.resize(mNumberOfVariables);;
  mVarLowerBound.resize(mNumberOfVariables);
  mVarUpperBound.resize(mNumberOfVariables);

  for(int i = 0;i< mNumberOfVariables; i++){
    mVarLowerBound[i] = -5.0;
    mVarUpperBound[i] = 5.0;
    mCstep[i] = 2.0;
  }

  mRank = rank;

  for(int i = 0; i< mNumberOfVariables; i++) mVm[i] = 1.0;
}

SimAnneal::SimAnneal(int numberOfVariables, ObjFunc * pObjFunc,  MPI_Comm& condComm, int rank){

  mNumberOfVariables = numberOfVariables;
  mpObjFunc = pObjFunc;
  mMpiComm = condComm;

  mTolerance = .05;
  mRt = .8;//
  mSimAnnEps = 1.0E-4;//
  
  mNs = 12;//
  mNt = 3;//
  mCheck = 2;//
  mMaxFcnEval = 20000;//
    
  mUratio = .6;
  mLratio = .4;
  mTemp = 5.0;

  mVm.resize(mNumberOfVariables);;//
  mCstep.resize(mNumberOfVariables);;
  mVarLowerBound.resize(mNumberOfVariables);
  mVarUpperBound.resize(mNumberOfVariables);

  for(int i = 0;i< mNumberOfVariables; i++){
    mVarLowerBound[i] = -5.0;
    mVarUpperBound[i] = 5.0;
    mCstep[i] = 2.0;
  }

  mRank = rank;

  for(int i = 0; i< mNumberOfVariables; i++) mVm[i] = 1.0;
}

SimAnneal::SimAnneal(){
  mMpiComm = mMpiComm;
  mNumberOfVariables = 0;//
  mpObjFunc = NULL;

  mTolerance = .05;
  mRt = .8;//
  mSimAnnEps = 1.0E-4;//
  
  mNs = 12;//
  mNt = 3;//
  mCheck = 2;//
  mMaxFcnEval = 20000;//
    
  mUratio = .6;
  mLratio = .4;
  mTemp = 5.0;

  mVm.resize(mNumberOfVariables);;//
  mCstep.resize(mNumberOfVariables);;
  mVarLowerBound.resize(mNumberOfVariables);
  mVarUpperBound.resize(mNumberOfVariables);

  for(int i = 0;i< mNumberOfVariables; i++){
    mVarLowerBound[i] = -5.0;
    mVarUpperBound[i] = 5.0;
    mCstep[i] = 2.0;
  }

  for(int i = 0; i< mNumberOfVariables; i++) mVm[i] = 1.0;
}

void SimAnneal::PrintInfo(ostream& outs){
  outs << left;
  outs << setw(25) << "Number of parameters: " << mNumberOfVariables << endl;
  outs << setw(25) << "Tol: " << mTolerance <<endl;
  outs << setw(25) << "Inital temp: " << mTemp << endl;
  outs << setw(25) << "rt: " << mRt << endl;             
  outs << setw(25) << "simanneps: " << mSimAnnEps << endl;
  outs << setw(25) << "ns: " << mNs << endl;
  outs << setw(25) << "nt: " << mNt << endl;
  outs << setw(25) << "check: " << mCheck << endl;
  outs << setw(25) << "max_func_eval: " << mMaxFcnEval <<endl;
  outs << setw(25) << "uratio: " << mUratio <<endl;
  outs << setw(25) << "lration: " << mLratio <<endl;
  outs << setw(25) << "vm initial: " << "[";
  for (int i=0; i<mVm.size(); i++){
    outs << mVm[i];
    if (i==mNumberOfVariables-1) outs << "]";
    else outs << "," ;
  }
  outs << endl;

  outs << setw(25) << "lower bound: " << "[";
  for (int i=0; i<mVarLowerBound.size(); i++){
    outs << mVarLowerBound[i];
    if (i==mNumberOfVariables-1) outs << "]";
    else outs << "," ;
  }
  outs <<endl;

  outs << setw(25) << "upper bound: " << "[";
  for (int i=0; i<mVarUpperBound.size(); i++){
    outs << mVarUpperBound[i];
    if (i==mNumberOfVariables-1) outs << "]";
    else outs << ",";
  }
  outs << endl;

  outs << setw(25) << "cstep: " << "[";
  for (int i=0; i<mCstep.size(); i++){
    outs << mCstep[i];
    if (i==mNumberOfVariables-1) outs << "]";
    else outs << ",";
  }
  outs <<endl;
}


void SimAnneal::SetParams(map<string, vector<string> > inputParams){

  OptimMethod::SetParams(inputParams);
  map<string, vector<string> >::iterator iter;
  string           param;
  vector<string>   pvalue;

  for (iter = inputParams.begin(); iter != inputParams.end(); iter++){
    param = iter->first;
    pvalue = iter->second;

    if      (param == "optim_tolerance")       SetTolerance( atof(&pvalue[0][0]));
    else if (param == "rt")                    SetRt( atof(&pvalue[0][0]));
    else if (param == "simanneps")             SetSimAnnEps( atof(&pvalue[0][0]));
    else if (param == "ns")                    SetNs( atoi(&pvalue[0][0]));
    else if (param == "nt")                    SetNt( atoi(&pvalue[0][0]));
    else if (param == "check")                 SetCheck( atoi(&pvalue[0][0]));
    else if (param == "max_func_eval")         SetMaxFcnEval(atoi(&pvalue[0][0]));
    else if (param == "uratio")                SetUratio( atof(&pvalue[0][0]));
    else if (param == "lration")               SetLratio( atof(&pvalue[0][0]));
    else if (param == "inital_temp")           SetInitTemp( atof(&pvalue[0][0]));

    else if (param == "vm_initial") {
      vector<float> vm_(pvalue.size(), 1);
      for (int i=0; i<pvalue.size(); i++) vm_[i] = atof(&pvalue[i][0]);
      SetInitMaxStep(vm_);
    }

    else if (param == "cstep") {
      vector<float> cstep_(pvalue.size(), 1);
      for (int i=0; i<pvalue.size(); i++) cstep_[i] = atof(&pvalue[i][0]);
      SetInitCstep(cstep_);
    }
  }
}



