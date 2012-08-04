load my_split.mat
load my_shared.mat
send_jobs_to_workers('Rinth_testfunc', 'NETWORK', {'aStation', 'tlMisfit_sub'}, {'srModel'})
