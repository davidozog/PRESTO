function mworker(my_hostname, rank)

  DEBUG = 0;

  % Set up a custom directory for the PCT
  %hostname = getenv('HOSTNAME');
  home     = getenv('HOME');
  hostpool = sprintf( '%s/.matlab/local_scheduler_data/R2011b/%s' ,home, my_hostname);
  sch = findResource('scheduler','type','local');
  set(sch, 'DataLocation', hostpool);

  import java.io.*;
  import java.net.*;

  % Connect to Python Master via a socket:
  try
    sock = Socket(my_hostname, 11110);
    in = BufferedReader(InputStreamReader(sock.getInputStream));
    out = PrintWriter(sock.getOutputStream,true);
  catch ME
    error(ME.identifier, 'Worker Connection Error: %s', ME.message)
  end

  % Send alive signal to the master 
  mesg = 'alive';
  out.println(mesg);

    run = 1;
    while run

      try
        fromClient = in.readLine();
      catch
        continue
      end
      fromClient = char(fromClient);
      if(DEBUG); fprintf(2, horzcat('Worker:(received): ', fromClient, '\n')); end

      % Parse the function name and the input filenames:
      if(DEBUG); fprintf(2, ['fromClient is ',  fromClient, '\n']); end
      [token, remain] = strtok(fromClient, ',');
      fh = str2func(strtrim(token));
      function_name = strtrim(token);
      [token, remain] = strtok(remain, ',');
      proto = strtrim(token);
      [token, remain] = strtok(remain, ',');
      jobid = strtrim(token)
      fprintf(2, ['jobid is ', jobid, '\n']);
      sf = strfind(remain, ',');
      arg_files = strtrim(remain(sf+1:end));
      [token, remain] = strtok(arg_files, ',');
      split_file = strtrim(token)
      if(DEBUG); fprintf(2, ['split_file is ',  split_file, '\n']); end
      sf = strfind(remain, ',');
      shared_file = strtrim(remain(sf+1:end))
      sf = strfind(shared_file, ',')
      if sf
        shared_file = strtrim(shared_file(1:sf-1))
        parmode = true
      end  
      
      


      % If protocol is shmem/network, get the data:
      if strcmp(proto, 'NETWORK')
        yrinth_str = 'm_a_t_l_a_b__y_r_i_n_t_h_';
        [token, remain] = strtok(remain, ':');
        shmem_size = strtrim(token)
        if(DEBUG); fprintf(2, ['\n Calling shmem2mat.c with shmem_size: ', shmem_size , '\n']); end
        stingray_data = shmem2mat(str2num(shmem_size));
        SD = deserialize(stingray_data)

        % Place objects in workspace:
        arg_str = '';
        for i = 1:length(SD)          
          assignin('base', [yrinth_str,num2str(i)], SD{i});   
          if i == 1
            arg_str = [arg_str, yrinth_str, num2str(i)];
          else
            arg_str = [arg_str, ', ', yrinth_str, num2str(i)];
          end
        end

        % Call the function:
        %[ret1 ret2]  = fh(SD{1}, SD{2}, SD{3})
        [ret1 ret2] = evalin('base', [function_name, '(', arg_str, ')']); 

        % Write results to shmem and pass to python worker process 
        ret = serialize({ret1, ret2})
        out.println(['shmem_data:', num2str(length(ret)), ':', jobid]);
        mat2shmem(ret);

      % TMPFS protocol:
      elseif strcmp(proto, 'TMPFS')
        TMPFS_PATH = '/dev/shm/';
        yrinth_str = 'm_a_t_l_a_b__y_r_i_n_t_h_';
        evalin('base', ['clear; load(''',split_file,''')']);
        evalin('base', 'W = whos');
        num_objs = evalin('base', 'length(W)');
        % Place objects in workspace:
        arg_str = '';
        for i = 1:num_objs
          if i == 1
            arg_str = [arg_str, yrinth_str, num2str(i)];
          else
            arg_str = [arg_str, ', ', yrinth_str, num2str(i)];
          end
        end
        arg_str
        [ret1 ret2] = evalin('base', [function_name, '(', arg_str, ')']); 

        % write results to file and pass path to master
        results_file = [TMPFS_PATH, '.results_', jobid, '.mat'];
        save(results_file, 'ret1', 'ret2');
        out.println(results_file);
        system(['rm ', split_file, ' ', results_file]);

      % NFS protocol:
      elseif strcmp(proto, 'NFS')
        [ret1 ret2]  = fh(split_file, shared_file);

        % write results to file and pass path to master
        results_file = ['./.results_', jobid, '.mat'];
        save(results_file, 'ret1', 'ret2');
        out.println(results_file);
        system(['rm ', split_file]);

      else
        fprintf(2, 'ERROR: Unrecognized protocol.\n')
      end

    end
