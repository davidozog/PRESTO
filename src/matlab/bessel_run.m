function [c d] = bessel_run(dcs, N)

%N = 100;
a = linspace(0,1,N);
b = 3;

if dcs == false

  %b = zeros(1,N);
  
  %parfor i=1:N
  %  b(i) = bessel_test( a(i) );
  %end
  
  tic
  [c d] = send_jobs_to_workers('bessel_test', {'a'}, {'b','dcs'}, true, true);
  toc
  
else

  tic
  parfor i=1:N
    [c(i) d(i)] = bessel_test( a(i), b, true );
  end
  toc

end 

c(1);
c(2);
c(N);

