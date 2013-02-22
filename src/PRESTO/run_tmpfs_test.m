load my_split10.mat
load my_shared.mat
tic
[A0 tlMisfit] = send_jobs_to_workers('Rinth_testfunc', {'aStation', 'tlMisfit_sub'}, {'srModel'});
toc

%a = [4 5 6 4 5 6 4 5 6];
%b = [1 2 3 4 5 6 7 8 9];
%c = 7;
%
%%x = send_jobs_to_workers('path_test_func', 'TMPFS', {'a', 'b'},  {'c'}, true) 
%send_jobs_to_workers('path_test_func', {'a', 'b'},  {'c'}, true, true) 

%kill_workers
%exit
