%dbstop in send_jobs_to_workers at 123
load my_split.mat
load my_shared.mat
tic
[A0 tlMisfit] = send_jobs_to_workers('Rinth_testfunc', 'NETWORK', {'aStation', 'tlMisfit_sub'}, {'srModel'})
toc
