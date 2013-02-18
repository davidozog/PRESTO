function [y c] = bessel_test(x,b)

  parfor i=1:length(x)
    y(i) = sqrt(1+x(i)*x(i)) * besselj(.25, x(i)) + exp(1/(x(i)+1))*ellipke(1/(1+log(x(i))^2));
    c(i) = b;
  end
