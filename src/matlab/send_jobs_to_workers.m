function [A B] = send_jobs_to_workers(remote_method, varargin)

  nVarargs = length(varargin);

  % Mode 1 : user supplied his own 'split' and 'shared' .mat files
  % Mode 2 : launch a single job with the given objects ...
  firstvar = varargin{1};
  mode=1;
  try
    if firstvar(end-3:end) == '.mat'
      mode = 1;
    else mode = 2;
    end
  catch
    mode=2;
  end

  if mode == 1
    load(varargin{1})
    W = whos;
    %num_jobs = length(W(1).size);
    num_jobs = W(1).size(1);
    aStation_ = aStation;
    tlMisfit_sub_ = tlMisfit_sub;
    mesg{num_jobs} = '';
    for j = 1:num_jobs
      jobid = int2str(j);
      aStation = aStation_(j);
      tlMisfit_sub = tlMisfit_sub_(j);
      save(horzcat('.split_', jobid, '.mat'), 'aStation', 'tlMisfit_sub');
      mesg{j} = horzcat(remote_method, ', ', jobid, ', ', './.split_', jobid, '.mat, ', varargin{2});
    end

  elseif mode==2
    num_jobs = 1;
    mesg{num_jobs} = '';
    save_str = '';
    for i=1:nVarargs
      save_str = [save_str ', ''' varargin{i}, ''''];
    end
    mpath = 'masterdata.mat';
    epath = 'empty.mat';
    evalin('caller', horzcat('save(''', mpath, '''', save_str, ')')) ;
    nul___ = '';
    save(epath, 'nul___');
    mesg{1} = horzcat(remote_method, ', 1, ', epath, ', ', mpath);
  end

  import java.io.*;
  import java.net.*;

  % Send job message to the Python master rank:
  try
    sock = Socket('localhost', 11112);
    in = BufferedReader(InputStreamReader(sock.getInputStream));
    out = PrintWriter(sock.getOutputStream,true);
  catch ME
    error(ME.identifier, 'Master Connection Error: %s', ME.message)
  end

  for i=1:num_jobs
    out.println(mesg{i});
  end

  out.println('done');

  % Receive all finished jobs from workers:
  results = cell(1,num_jobs);
  jobs_accounted_for = 0;
  while ( jobs_accounted_for < num_jobs )
    try
      fromMPI = in.readLine();
    catch
      pause(1)
      continue
    end
    fromMPI = char(fromMPI);
    if length(fromMPI>0)
      %fprintf(1, horzcat('Master:(received): ', fromMPI, '\n'));
      jobs_accounted_for = jobs_accounted_for + 1;
      results(jobs_accounted_for) = {fromMPI};
    end
  end

  % I have all the results now, so put them into the output objects 
  init_results(num_jobs);
  for i=1:length(results)
    result_file = char(results(i));
    load(result_file);
    [token, remain] = strtok(result_file, '_');
    idx = remain(2:strfind(remain, '.')-1);
    idx = str2num(idx);
    A(idx) = struct(ret1);
    B(idx) = struct(ret2);
    system(['rm ', result_file]);
  end

  sock.close();
