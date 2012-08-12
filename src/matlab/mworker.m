function mworker(my_hostname, rank)

  DEBUG = 0;

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
        pause(1)
        continue
      end
      fromClient = char(fromClient);
      if(DEBUG); fprintf(2, horzcat('Worker:(received): ', fromClient, '\n')); end

        % Set up a custom directory for the PCT
        %hostname = getenv('HOSTNAME');
        home     = getenv('HOME');
        hostpool = sprintf( '%s/.matlab/local_scheduler_data/R2011b/%s' ,home, my_hostname);
        sch = findResource('scheduler','type','local');
        set(sch, 'DataLocation', hostpool);

        % Parse the function name and the input filenames:
        fprintf(2, ['fromClient is ',  fromClient])
        [token, remain] = strtok(fromClient, ',');
        fh = str2func(strtrim(token));
        [token, remain] = strtok(remain, ',');
        proto = strtrim(token);
        [token, remain] = strtok(remain, ',');
        jobid = strtrim(token);
        sf = strfind(remain, ',')
        arg_files = strtrim(remain(sf+1:end))
        [token, remain] = strtok(arg_files, ',');
        split_file = strtrim(token)
        fprintf(2, ['split_file is ',  split_file])
        sf = strfind(remain, ',')
        shared_file = strtrim(remain(sf+1:end))

        % If protocol is shmem/network, get the data:
        if strcmp(proto, 'NETWORK')
          [token, remain] = strtok(remain, ':');
          shmem_size = strtrim(token)
          fprintf(2, ['\n > > > > > > > > > > CALLING shmem2mat.c with shmem_size: ', shmem_size , '\n'])
          %evalin('caller', ['stingray_data = shmem2mat(',shmem_size,')']);
          stingray_data = shmem2mat(str2num(shmem_size));
          SD = deserialize(stingray_data)
          SD{1}(rank)
          SD{3}

          % Call the function:
          [ret1 ret2]  = fh(SD{1}(rank), SD{3})

          % Write results to shmem and pass to python worker process 
          ret = serialize({ret1, ret2})
          out.println(['shmem_data:', num2str(length(ret))]);
          mat2shmem(ret);


        % NFS protocol:
        else 
          [ret1 ret2]  = fh(split_file, shared_file);

          % write results to file and pass path to master
          results_file = ['./.results_', jobid, '.mat'];
          save(results_file, 'ret1', 'ret2');
          out.println(results_file);
          system(['rm ', split_file]);

        end

    end
