load my_split10.mat
load my_shared.mat
shared2 = 'fdsa';
tic
[A0 tlMisfit] = send_jobs_to_workers('Rinth_testfunc_wrapper', 'NETWORK', {'aStation', 'tlMisfit_sub'}, {'srModel', 'shared2'}, true)
toc
