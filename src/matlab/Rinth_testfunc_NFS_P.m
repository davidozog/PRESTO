%function [A0_sub tlMisfit_sub] = Rinth_testfunc(aStation, srModel)
function [A0_sub tlMisfit_sub] = Rinth_testfunc_NFS(split_filepath, shared_filepath)
  load(split_filepath)
  load(shared_filepath)
  fprintf(2, horzcat('hi from matlab - shared_filepath is ', shared_filepath, ' \n'))

  aStation 
  aStation(1)
  aStation(1:5)
  tlMisfit_sub
  who

  srModel = srModel;
  myStations = aStation
  
  matlabpool open
    parfor i=1:5

    station = str2num(char(myStations(i)))
    A0_sub(i).A0 = [station station; station station] + srModel;
    tlMisfit_sub(i).bvec = [station];
    tlMisfit_sub(i).res = [station];
    tlMisfit_sub(i).ptr    = [station];
    tlMisfit_sub(i).ttime  = [station];
    tlMisfit_sub(i).inode  = [station];
    tlMisfit_sub(i).ray_minz   = [station];
    tlMisfit_sub(i).niter = station;
    tlMisfit_sub(i).icntTT = station;
    tlMisfit_sub(i).zssqr  = station;
    tlMisfit_sub(i).wssqr  = station;                                         
    tlMisfit_sub(i).sumsd  = station;
    tlMisfit_sub(i).nssqr  = station;                  
    tlMisfit_sub(i).nelsum = station;
    tlMisfit_sub(i).nramat = station;

  end
  matlabpool close
