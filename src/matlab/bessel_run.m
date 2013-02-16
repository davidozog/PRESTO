N = 1000000;
a = linspace(0,1,N);

%b = zeros(1,N);

%parfor i=1:N
%  b(i) = bessel_test( a(i) );
%end

[b c] = send_jobs_to_workers('bessel_test', 'TMPFS', {'a'}, {}, true);


b(1)
b(2)
b(N)
