function [A B] = send_jobs_to_workers(remote_method, varargin)

  DEBUG = 0;
  PPN = 1;

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

  %firstvar = varargin{1};
  mode=4;
  %try
  %  if strcmp(firstvar, 'NFS')
  %    mode = 1;
  %  elseif strcmp(firstvar, 'NETWORK')
  %    mode = 3;
  %  elseif strcmp(firstvar, 'TMPFS')
  %    mode = 4;
  %  else 
  %    mode = 2;
  %  end
  %catch
  %  mode=2;
  %end

  % 4th arg is whether or not to use Matlab PCT:
  try
    parvar = varargin{3};
    if varargin{3}
      parmode = true;
    else 
      parmode = false;
    end
  catch
    parmode = false;
  end

  % 6th arg is whether or not to bundle task groups together:
  try
    bundlevar = varargin{4};
    if varargin{4}
      bundlemode = true;
    else 
      bundlemode = false;
    end
  catch
    bundlemode = false;
  end

  % MODE 1:
  if mode==1
    load(varargin{1})
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
        mesg{j} = horzcat(remote_method, ', ', varargin{0}, ', ', jobid, ...
                          ', ', './.split_', jobid, '.mat, ', varargin{2}, ...
                          ', ', 'P');
      end

    else
      for j = 1:num_jobs
        jobid = int2str(j);
        aStation = aStation_(j);
        tlMisfit_sub = tlMisfit_sub_(j);
        save(horzcat('.split_', jobid, '.mat'), 'aStation', 'tlMisfit_sub');
        mesg{j} = horzcat(remote_method, ', ', varargin{0}, ', ', jobid, ...
                          ', ', './.split_', jobid, '.mat, ', varargin{2});
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

    yrinth_str = 'p_r_e_s_t_o_';

    if parmode
      error('PCT mode not supported with shared memory implementation');
    end

    % serialize a cell containing the split objects
    splitVarStr = '';
    num_split_objects = length(varargin{1});
    for i=1:num_split_objects
      splitVarStr = [splitVarStr, varargin{1}{i}, ', '];
    end

    % Get size of first variable and assume it's the number of jobs
    num_jobs = evalin('caller', ['length(', varargin{1}{1}, ')']);

    % serialize a cell containing the shared objects
    sharedVarStr = '';
    for i=1:length(varargin{2})
      sharedVarStr = [sharedVarStr, varargin{2}{i}, ', '];
    end

    shmem_size = zeros(num_jobs, 1);
    for i=1:num_jobs
      job_str = '';
      for j=1:num_split_objects
        job_str = [job_str, varargin{1}{j},'(',num2str(i),'),'];
      end
        evalin('caller', [yrinth_str,num2str(i),'=serialize({',job_str,...
               sharedVarStr,'});']);
        shmem_size(i) = evalin('caller', ['length(',yrinth_str,num2str(i),')']);
    end

    mesg{num_jobs} = '';
    for j = 1:num_jobs
      jobid = int2str(j);
      mesg{j} = horzcat(remote_method, ', ', varargin{0}, ', ', ...
                    jobid, ', ', './.split_', jobid, '.mat, ', ...
                    int2str(shmem_size(j)));
    end
    
  end % mode 3


  % MODE 4:
  if mode==4
    % Get user id for saving file objects:
    [status, uid] = system('id');
    [uid, remain] = strtok(uid, ' ');
    lfteq = strfind(uid, '=') + 1;
    rtparen = strfind(uid, '(') - 1;
    uid = uid(lfteq:rtparen);

    yrinth_str = 'p_r_e_s_t_o_';
    yrinth_str_shared = 'p_r_e_s_t_o_sh_';
    TMPFS_PATH = '/dev/shm/';

    % serialize a cell containing the split objects
    splitVarStr = '';
    num_split_objects = length(varargin{1});
    for i=1:num_split_objects
      splitVarStr = [splitVarStr, varargin{1}{i}, ', '];
    end
    %fprintf(1, ['splitVarStr is ', splitVarStr]);

    % Get size of first variable and assume it's the number of jobs
    num_jobs = evalin('caller', ['length(', varargin{1}{1}, ')']);
%    num_jobs = num_split_objects;

    if parmode

      if bundlemode
        num_workers = evalin('base', ['length(presto_workers)']);
        total_tasks = num_jobs;
        try
          skip = varargin{5};
          num_jobs = ceil(num_jobs / skip)
        catch
          skip = ceil(num_jobs / num_workers);
          num_jobs = num_workers;
        end
    
      else
        total_tasks = num_jobs
        if mod(num_jobs, PPN) == 0
          num_jobs = num_jobs/PPN;
        else
          num_jobs = floor(num_jobs/PPN) + 1;
        end
        display(['Total Number of Jobs: ', num2str(num_jobs)]);

      end

      % Shared data:
      job_str_shared = '''';
      num_shared_objs = length(varargin{2});
      if num_shared_objs > 0
        for k=1:num_shared_objs
          splitID = int2str(k);
          evalin('caller', [yrinth_str_shared,splitID,'=', varargin{2}{k}, ';']);
          job_str_shared = [job_str_shared, yrinth_str_shared, splitID, ''', '''];
        end
        job_str_shared = job_str_shared(1:end-3);
        save_str = ['''', TMPFS_PATH, '.', uid, '_sh.mat'', ', job_str_shared];
        evalin('caller', ['save(', save_str , ')']);
      end

      for i = 1:num_jobs
        job_str = '''';
        mesg{num_jobs} = '';
        jobid = int2str(i);
        if bundlemode
          fst = int2str((i-1)*skip+1);
          if (i-1)*skip+skip <= total_tasks
            lst = int2str((i-1)*skip+skip);
          else
            lst = int2str(total_tasks);
          end
        else
          fst = int2str((i-1)*PPN+1);
          if (i-1)*PPN+PPN <= total_tasks
            lst = int2str((i-1)*PPN+PPN);
          else
            lst = int2str(total_tasks);
          end
        end
        jobrng = [fst,':',lst];

        for j=1:num_split_objects
          splitID = int2str(j);
          job_str = [job_str, yrinth_str, splitID, ''', '''];
          evalin('caller', [yrinth_str,splitID,'=', varargin{1}{j},'(',jobrng,');']);
        end
        job_str = job_str(1:end-3);

        save_str = ['''', TMPFS_PATH, '.', uid, '_sp_', jobid, '.mat'', ', job_str];
        evalin('caller', ['save(', save_str , ')']);

        mesg{i} = [remote_method, ', TMPFS, ', jobid, ...
                          ', ', TMPFS_PATH, '.', uid, '_sp_', jobid, '.mat, ', int2str(num_shared_objs)];
      end

    else
      display(['Total Number of Jobs: ', num2str(num_jobs)]);

      % Shared data:
      job_str_shared = '''';
      num_shared_objs = length(varargin{2})
      for k=1:num_shared_objs
        splitID = int2str(k);
        evalin('caller', [yrinth_str_shared,splitID,'=', varargin{2}{k}, ';']);
        job_str_shared = [job_str_shared, yrinth_str_shared, splitID, ''', '''];
      end
      if num_shared_objs > 0
        job_str_shared = job_str_shared(1:end-3);
        save_str = ['''', TMPFS_PATH, '.', uid, '_sh.mat'', ', job_str_shared];
        evalin('caller', ['save(', save_str , ')']);
      end

      for i=1:num_jobs
        job_str = '''';
        mesg{num_jobs} = '';
        jobid = int2str(i);
        for j=1:num_split_objects
          splitID = int2str(j);
          job_str = [job_str, yrinth_str, splitID, ''', '''];
          evalin('caller', [yrinth_str,splitID,'=', varargin{1}{j},'(',jobid,');']);
        end
        job_str = job_str(1:end-3);

        save_str = ['''', TMPFS_PATH, '.', uid, '_sp_', jobid, '.mat'', ', job_str];
        evalin('caller', ['save(', save_str , ')']);

        mesg{i} = [remote_method, ', TMPFS, ', jobid, ...
                          ', ', TMPFS_PATH, '.', uid, '_sp_', jobid, '.mat, ', int2str(num_shared_objs)];
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
    mesg{i};
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
      %A(i) = D{1};
      %B(i) = D{2};
      fprintf(2, 'ERROR: This mode isnt supported anymore.\n')
    end 
  else 
    
    A = zeros(1,total_tasks);
    B = zeros(1,total_tasks);

    for i=1:num_results
      result_file = char(results(i));
      load(result_file);
      [token, remain] = strtok(result_file, 'r');
      idx = remain(2:strfind(remain, '.')-1);
      idx = str2num(idx);
      if parmode
        if bundlemode
          if idx ~= num_jobs 
            A(skip*(idx-1)+1:skip*(idx-1)+skip) = ret1(1:skip);
            B(skip*(idx-1)+1:skip*(idx-1)+skip) = ret2(1:skip);
          else
            A(skip*(idx-1)+1:total_tasks) = ret1(1:length(ret1));
            B(skip*(idx-1)+1:total_tasks) = ret2(1:length(ret2));
          end
          
%          for j=1:skip
%            myid = skip*(idx-1)+j;
%            if idx ~= num_jobs 
%              A(myid) = ret1(j);
%              B(myid) = ret2(j);
%              %for k=1:nargout
%              %  eval(['varargout{k}(skip*(idx-1)+j) = ret',num2str(k),'(j);']);
%              %end
%            elseif myid <= total_tasks
%              A(myid) = ret1(j);
%              B(myid) = ret2(j);
%              %for k=1:nargout
%              %  eval(['varargout{k}(skip*(idx-1)+j) = ret',num2str(k),'(j);']);
%              %end
%            end
%          end
        else
          for j=1:PPN
            %A(PPN*(idx-1)+j) = struct(ret1(j));
            %B(PPN*(idx-1)+j) = struct(ret2(j));
            % Might need to be struct for Stingray:
            if idx ~= num_jobs 
              for k=1:nargout
                eval(['varargout{k}(PPN*(idx-1)+j) = ret',num2str(k),'(j);']);
              end
            elseif PPN*(idx-1)+j <= total_tasks
              for k=1:nargout
                eval(['varargout{k}(PPN*(idx-1)+j) = ret',num2str(k),'(j);']);
              end
            end
          end
        end
      else 
        for k=1:nargout
          eval(['varargout{k}(idx) = ret', num2str(k),';']);
        end
      end
      system(['rm ', result_file]);
    end

  end

  sock.close();
  evalin('caller', 'clearvars p_r_e_s_t_o_*');
