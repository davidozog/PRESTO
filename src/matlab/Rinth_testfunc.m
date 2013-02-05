function [A0_sub tlMisfit_sub] = Rinth_testfunc(aStation, tlMisfit_sub, srModel)
%function [A0_sub tlMisfit_sub] = Rinth_testfunc(split_filepath, shared_filepath)
%  load(split_filepath)
%  load(shared_filepath)
  fprintf(2, horzcat('hi from matlab - aStation is ', char(aStation), ' \n'))

  station = str2num(char(aStation));
  A0_sub.A0 = [station station; station station] + srModel;
  tlMisfit_sub.bvec = [station];
  tlMisfit_sub.res = [station];
  tlMisfit_sub.ptr    = [station];
  tlMisfit_sub.ttime  = [station];
  tlMisfit_sub.inode  = [station];
  tlMisfit_sub.ray_minz   = [station];
  tlMisfit_sub.niter = station;
  tlMisfit_sub.icntTT = station;
  tlMisfit_sub.zssqr  = station;
  tlMisfit_sub.wssqr  = station;                                         
  tlMisfit_sub.sumsd  = station;
  tlMisfit_sub.nssqr  = station;                  
  tlMisfit_sub.nelsum = station;
  tlMisfit_sub.nramat = station;
