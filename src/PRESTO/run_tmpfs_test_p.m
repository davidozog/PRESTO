load my_split10.mat
load my_shared.mat
shared2 = 'fdsa';
tic
[A0 tlMisfit] = send_jobs_to_workers('Rinth_testfunc_P', {'aStation', 'tlMisfit_sub'}, {'srModel', 'shared2'}, true, true, 2);
%[A0 tlMisfit] = send_jobs_to_workers('Rinth_testfunc_P', 'TMPFS', {aStation, tlMisfit_sub}, {srModel, shared2}, true)
toc
