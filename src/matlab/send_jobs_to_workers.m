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
    %NSplit = find(strcmp(varargin{2}, 'shared'));
    %stingrayObj(varargin{2}(2:NSplit-1), varargin{2}(NSplit+1:end));

    % serialize a cell containing the split objects
    splitVarStr = '';
    num_split_objects = length(varargin{2});
    for i=1:num_split_objects
      splitVarStr = [splitVarStr, varargin{2}{i}, ', '];
    end
    %fprintf(1, ['splitVarStr is ', splitVarStr]);

    % Get size of first variable and assume it's the number of jobs
    num_jobs = evalin('caller', ['length(', varargin{2}{1}, ')']);

    % serialize a cell containing the shared objects
    sharedVarStr = '';
    for i=1:length(varargin{3})
      sharedVarStr = [sharedVarStr, varargin{3}{i}, ', '];
    end

    shmem_size = zeros(num_jobs, 1);
    for i=1:num_jobs
      job_str = '';
      for j=1:num_split_objects
        job_str = [job_str, varargin{2}{j},'(',num2str(i),'),']
      end
        evalin('caller', ['m',num2str(i),'=serialize({',job_str,...
               sharedVarStr,'});']);
        shmem_size(i) = evalin('caller', ['length(m',num2str(i),')']);
    end

    mesg{num_jobs} = '';
    for j = 1:num_jobs
      jobid = int2str(j);
      mesg{j} = horzcat(remote_method, ', ', varargin{1}, ', ', ...
                    jobid, ', ', './.split_', jobid, '.mat, ', ...
                    int2str(shmem_size(j)));
    end
    
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

  % This is a 'done' message.  Don't change it.
  out.println('done');

  if mode==3
    fprintf('before master shmem');
    for i=1:num_jobs
      evalin('caller', ['mat2shmemQ( m', num2str(i), ', ', ...
                num2str(shmem_size(i)), ', ', num2str(i), ')']);
    end
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
      fprintf(1, horzcat('Master:(received): ', fromMPI, '\n'));
      jobs_accounted_for = jobs_accounted_for + 1;
      results(jobs_accounted_for) = {fromMPI};
    end
  end

  % I have all the results now, so put them into the output objects 
  %TODO: split this loop and call shmResult() with a list of (jobid, datasize) pairs
  %      which returns when all data is read.
  if mode==3
    %res_arg = zeros(1,length(results));
    for i=1:length(results)
      [token, remain] = strtok(results(i), ':');
      [token, remain] = strtok(remain, ':');
      shmem_size = strtrim(token);
      [token, remain] = strtok(remain, ':');
      %TODO: This should be jobid, not rank.  Trace it down and make sure that's the case
      result_rank = strtrim(token); 
      res_arg{str2num(char(result_rank))} = [str2num(char(shmem_size)), str2num(char(result_rank))];
      %evalin('caller', ['r',num2str(i),'=[',char(shmem_size), ', ', char(result_rank),'];']);
    end
    
      
    fprintf(1, ['Master GETTING RESULTS ...']);
    % TODO: These have to be sorted first!
    for i=1:length(results)
      R{i} = shmResult2mat(res_arg{i})
    end

    for i=1:length(results)
      D = deserialize(R{i});
      A(i) = D{1};
      B(i) = D{2};
    end

  else 
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

  end

  sock.close();
