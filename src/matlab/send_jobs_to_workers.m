function [A B] = send_jobs_to_workers(remote_method, varargin)

  DEBUG = 1;
  PPN = 5;

  nVarargs = length(varargin);

  % Mode 1 :
  %  Supplied 'split' and 'shared' .mat files.  For example:
  %      send_jobs_to_workers('myfunc', 'NFS', 'my_split.mat', 'my_shared.mat')
  %
  % Mode 2 : 
  %  Launched a single job with the given objects.  For example:
  %      send_jobs_to_workers('myfunc', 'object1', 'object2', ...)
  %  TODO: Make this mode asynchronous...
  %
  % Mode 3 : 
  %  Pass a list of split and shared object names. For example, 
  %      send_jobs_to_workers('myfunc', 'NETWORK', ...
  %                           {'split_object1', 'split_object2', ...}, ...
  %                           {'shared_object1', 'shared_object2', ...})
  %  This will serialize the collection of data messages and send
  %  them over the network through shared memory.
  %
  % Mode 4 :
  %  Keeps .mat object files in memory by writing to tmpfs and sending
  %  them as buffers over MPI.  For example,
  %      send_jobs_to_workers('myfunc', 'TMPFS', {'split1', 'split2'}, ...
  %                           {'shared1', 'shared2'})

  firstvar = varargin{1};
  mode=1;
  try
    if strcmp(firstvar, 'NFS')
      mode = 1;
    elseif strcmp(firstvar, 'NETWORK')
      mode = 3;
    elseif strcmp(firstvar, 'TMPFS')
      mode = 4;
    else 
      mode = 2;
    end
  catch
    mode=2;
  end

  try
    parvar = varargin{4}
    if varargin{4}
      parmode = true
    else 
      parmode = false
    end
  catch
    parmode = false
  end

  % MODE 1:
  if mode==1
    load(varargin{2})
    %W = whos;
    %num_jobs = length(W(1).size);
    %num_jobs = W(1).size(1)
    num_jobs = length(aStation)
    aStation_ = aStation;
    tlMisfit_sub_ = tlMisfit_sub;
    mesg{num_jobs} = '';
    % Split the split_file into individual jobs:
    if parmode
    
      if mod(num_jobs, PPN) == 0
        num_jobs = num_jobs/PPN;
      else
        num_jobs = num_jobs/PPN + 1;
      end

      for j = 1:num_jobs
        jobid = int2str(j);
        aStation = aStation_( (j-1)*PPN+1 : ((j-1)*PPN+PPN) );
        tlMisfit_sub = tlMisfit_sub_( (j-1)*PPN+1 : ((j-1)*PPN+PPN) );
        save(horzcat('.split_', jobid, '.mat'), 'aStation', 'tlMisfit_sub');
        mesg{j} = horzcat(remote_method, ', ', varargin{1}, ', ', jobid, ...
                          ', ', './.split_', jobid, '.mat, ', varargin{3}, ...
                          ', ', 'P');
      end

    else
      for j = 1:num_jobs
        jobid = int2str(j);
        aStation = aStation_(j);
        tlMisfit_sub = tlMisfit_sub_(j);
        save(horzcat('.split_', jobid, '.mat'), 'aStation', 'tlMisfit_sub');
        mesg{j} = horzcat(remote_method, ', ', varargin{1}, ', ', jobid, ...
                          ', ', './.split_', jobid, '.mat, ', varargin{3});
      end
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

    yrinth_str = 'm_a_t_l_a_b__y_r_i_n_t_h_';

    if parmode
      error('PCT mode not supported with shared memory implementation');
    end

    % serialize a cell containing the split objects
    splitVarStr = '';
    num_split_objects = length(varargin{2});
    for i=1:num_split_objects
      splitVarStr = [splitVarStr, varargin{2}{i}, ', '];
    end

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
        job_str = [job_str, varargin{2}{j},'(',num2str(i),'),'];
      end
        evalin('caller', [yrinth_str,num2str(i),'=serialize({',job_str,...
               sharedVarStr,'});']);
        shmem_size(i) = evalin('caller', ['length(',yrinth_str,num2str(i),')']);
    end

    mesg{num_jobs} = '';
    for j = 1:num_jobs
      jobid = int2str(j);
      mesg{j} = horzcat(remote_method, ', ', varargin{1}, ', ', ...
                    jobid, ', ', './.split_', jobid, '.mat, ', ...
                    int2str(shmem_size(j)));
    end
    
  end % mode 3


  % MODE 4:
  if mode==4
    yrinth_str = 'm_a_t_l_a_b__y_r_i_n_t_h_';
    TMPFS_PATH = '/dev/shm/';

    % serialize a cell containing the split objects
    splitVarStr = '';
    num_split_objects = length(varargin{2})
    for i=1:num_split_objects
      splitVarStr = [splitVarStr, varargin{2}{i}, ', '];
    end
    %fprintf(1, ['splitVarStr is ', splitVarStr]);

    % Get size of first variable and assume it's the number of jobs
    num_jobs = evalin('caller', ['length(', varargin{2}{1}, ')']);
    if(DEBUG); fprintf(1, ['num_jobs is : ', num2str(num_jobs), '\n']); end
    if parmode
    
      if mod(num_jobs, PPN) == 0
        num_jobs = num_jobs/PPN;
      else
        num_jobs = num_jobs/PPN + 1;
      end

      for i = 1:num_jobs

        job_str = '''';
        mesg{num_jobs} = '';
        jobid = int2str(i);
        fst = int2str((i-1)*PPN+1);
        lst = int2str((i-1)*PPN+PPN);
        jobrng = [fst,':',lst]

        for j=1:num_split_objects
          splitID = int2str(j);
          job_str = [job_str, yrinth_str, splitID, ''', '''];
          evalin('caller', [yrinth_str,splitID,'=', varargin{2}{j},'(',jobrng,');']);
        end
        for k=1:length(varargin{3})
          splitID = int2str(k+j);
          evalin('caller', [yrinth_str,splitID,'=', varargin{3}{k}, ';']);
          job_str = [job_str, yrinth_str, splitID, ''', '''];
        end
        job_str = job_str(1:end-3);

        save_str = ['''', TMPFS_PATH, '.jd_', jobid, '.mat'', ', job_str];
        evalin('caller', ['save(', save_str , ')']);

        mesg{i} = [remote_method, ', ', varargin{1}, ', ', jobid, ...
                          ', ', TMPFS_PATH, '.jd_', jobid, '.mat, P'];

      end

    else
      for i=1:num_jobs
        job_str = '''';
        mesg{num_jobs} = '';
        jobid = int2str(i);
        for j=1:num_split_objects
          splitID = int2str(j);
          job_str = [job_str, yrinth_str, splitID, ''', '''];
          evalin('caller', [yrinth_str,splitID,'=', varargin{2}{j},'(',jobid,');']);
        end
        for k=1:length(varargin{3})
          splitID = int2str(k+j);
          evalin('caller', [yrinth_str,splitID,'=', varargin{3}{k}, ';']);
          job_str = [job_str, yrinth_str, splitID, ''', '''];
        end
        job_str = job_str(1:end-3);

        save_str = ['''', TMPFS_PATH, '.jd_', jobid, '.mat'', ', job_str];
        evalin('caller', ['save(', save_str , ')']);

        mesg{i} = [remote_method, ', ', varargin{1}, ', ', jobid, ...
                          ', ', TMPFS_PATH, '.jd_', jobid, '.mat,'];
      end
    end
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
    mesg{i}
    out.println(mesg{i});
  end

  % This is the 'done' message.  Don't change it.
  out.println('done');

  if mode==3
    for i=1:num_jobs
      evalin('caller', ['mat2shmemQ(',yrinth_str,num2str(i), ', ', ...
                num2str(shmem_size(i)), ', ', num2str(i), ')']);
    end
  end

  % Receive all finished jobs from workers:
  results = cell(1,num_jobs);
  jobs_accounted_for = 0;
  while ( jobs_accounted_for < num_jobs )
    try
      fromMPI = in.readLine();
    catch
      continue
    end
    fromMPI = char(fromMPI);
    if length(fromMPI>0)
      if(DEBUG); fprintf(1, horzcat('Master:(received): ', fromMPI, '\n')); end
      jobs_accounted_for = jobs_accounted_for + 1;
      results(jobs_accounted_for) = {fromMPI};
    end
  end

  num_results = length(results);

  % I have all the results now, so put them into the output objects 
  %TODO: split this loop and call shmResult() with a list of (jobid, datasize) pairs
  %      which returns when all data is read.
  if mode==3
    %res_arg = zeros(1,length(results));
    for i=1:num_results
      [token, remain] = strtok(results(i), ':');
      [token, remain] = strtok(remain, ':');
      shmem_size = strtrim(token);
      [token, remain] = strtok(remain, ':');
      %TODO: This should be jobid, not rank.  Trace it down and make sure that's the case
      result_rank = strtrim(token); 
      res_arg{str2num(char(result_rank))} = [str2num(char(shmem_size)), str2num(char(result_rank))];
      %evalin('caller', ['r',num2str(i),'=[',char(shmem_size), ', ', char(result_rank),'];']);
    end
    

    % Getting results...
    for i=1:num_results
      R{i} = shmResult2mat(res_arg{i});
    end

    for i=1:num_results
      D = deserialize(R{i});
      A(i) = D{1};
      B(i) = D{2};
    end

  else 
    for i=1:num_results
      result_file = char(results(i));
      load(result_file);
      [token, remain] = strtok(result_file, '_');
      idx = remain(2:strfind(remain, '.')-1);
      idx = str2num(idx);
      ret1
      if parmode
        for j=1:PPN
          A(PPN*(idx-1)+j) = struct(ret1(j));
          B(PPN*(idx-1)+j) = struct(ret2(j));
        end
      else 
        A(idx) = struct(ret1);
        B(idx) = struct(ret2);
      end
      system(['rm ', result_file]);
    end

  end

  sock.close();
