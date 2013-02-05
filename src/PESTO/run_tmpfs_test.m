load my_split10.mat
load my_shared.mat
tic
[A0 tlMisfit] = send_jobs_to_workers('Rinth_testfunc', 'TMPFS', {'aStation', 'tlMisfit_sub'}, {'srModel'});
toc
