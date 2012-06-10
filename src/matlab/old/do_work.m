function ret = do_work(rank)

  %matlabpool open 8
  
  a = zeros(1,8);
  %parfor i=1:8
  for i=1:8
    %pause(5)
    a(i) = i*i*i*i*i;
  end
  
  %matlabpool close

ret = rank
