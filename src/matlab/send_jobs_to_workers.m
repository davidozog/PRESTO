function [A B] = send_jobs_to_workers(remote_method, varargin)

  nVarargs = length(varargin);

  % Mode 1 : user supplied his own 'split' and 'shared' .mat files
  %          i.e.  send_jobs_to_workers('myfunc', 'NFS', 'my_split.mat', 'my_shared.mat')
  % Mode 2 : launch a single job with the given objects ...
  %          i.e.  send_jobs_to_workers('myfunc', 'object1', 'object2', ...)
  % TODO: Make Mode 2 asynchronous...
  % Mode 3 : pass objects to a mex file
  %          i.e   send_jobs_to_workers('myfunc', 'NETWORK', {split: 'object1', 'object2', ...} 
  %                                               {shared: 'object3', 'object4', ...})
  firstvar = varargin{1};
  mode=1;
  try
    if strcmp(firstvar, 'NFS')
      mode = 1;
    elseif strcmp(firstvar, 'NETWORK')
      mode = 3;
    else 
      mode = 2;
    end
  catch
    mode=2;
  end

  % MODE 1:
  if mode==1
    load(varargin{2})
    W = whos;
    %num_jobs = length(W(1).size);
    num_jobs = W(1).size(1);
    aStation_ = aStation;
    tlMisfit_sub_ = tlMisfit_sub;
    mesg{num_jobs} = '';
    % Split the split_file into individual jobs:
    for j = 1:num_jobs
      jobid = int2str(j);
      aStation = aStation_(j);
      tlMisfit_sub = tlMisfit_sub_(j);
      save(horzcat('.split_', jobid, '.mat'), 'aStation', 'tlMisfit_sub');
      mesg{j} = horzcat(remote_method, ', ', varargin{1}, ', ', jobid, ', ', './.split_', jobid, '.mat, ', varargin{3});
    end
  end

  % MODE 2:
  % TODO: I don't think this will work anymore 
  % now that NETWORK/NFS is implemented...
  if mode==2
    num_jobs = 1;
    mesg{num_jobs} = '';
    save_str = '';
    for i=1:nVarargs
      save_str = [save_str ', ''' varargin{i}, ''''];
    end
    mpath = '.masterdata.mat';
    epath = '.empty.mat';
    evalin('caller', horzcat('save(''', mpath, '''', save_str, ')')) ;
    nul___ = '';
    save(epath, 'nul___');
    mesg{1} = horzcat(remote_method, ', 1, ', epath, ', ', mpath);
  end

  % MODE 3:
  if mode==3
  fprintf(1, 'got it');
    %NSplit = find(strcmp(varargin{2}, 'shared'));
    %stingrayObj(varargin{2}(2:NSplit-1), varargin{2}(NSplit+1:end));

    % serialize a cell containing the split objects
    splitVarStr = '';
    for i=1:length(varargin{2})
      splitVarStr = [splitVarStr, varargin{2}{i}, ', '];
    end
  fprintf(1, ['splitVarStr is ', splitVarStr]);

    % Get size of first variable and assume it's the number of jobs
    num_jobs = evalin('caller', ['length(', varargin{2}{1}, ')']);

    % serialize a cell containing the shared objects
    sharedVarStr = '';
    for i=1:length(varargin{3})
      sharedVarStr = [sharedVarStr, varargin{3}{i}, ', '];
    end

    shmem_size = zeros(num_jobs, 1);
    % TODO: make m1, m2, ... all separate splits of data
    for i=1:num_jobs
      evalin('caller', ['m',num2str(i),'=serialize({',splitVarStr,sharedVarStr,'});']);
      shmem_size(i) = evalin('caller', ['length(m',num2str(i),')']);
    end

    mesg{num_jobs} = '';
    for j = 1:num_jobs
      jobid = int2str(j);
      mesg{j} = horzcat(remote_method, ', ', varargin{1}, ', ', jobid, ', ', './.split_', jobid, '.mat, ', int2str(shmem_size(j)));
    end
    mesg{3}(1)

    
  end % mode 3

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

  % TODO bring back the [m1, m2, m3] version...
  if mode==3
    fprintf('before shmem');
    evalin('caller', ['mat2shmem( [m1, m2, m3])';]);
    %evalin('caller', ['mat2shmem(m1)';]);
    fprintf('after master shmem');
  end

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
  %init_results(num_jobs);
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
