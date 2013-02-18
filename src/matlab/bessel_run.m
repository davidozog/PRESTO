N = 90;
a = linspace(0,1,N);
b = 3;

%b = zeros(1,N);

%parfor i=1:N
%  b(i) = bessel_test( a(i) );
%end

[c d] = send_jobs_to_workers('bessel_test', 'TMPFS', {'a'}, {'b'}, true, true, 10);


c(1)
c(2)
c(N)
