function jserver(hostname, rank, funcname)

  pwd
  load split_data 
  load shared_data
  %load '/home/dave/School/Masters-Thesis/src/shared_data.mat'

  shared_struct
  split_struct

  import java.io.*;
  import java.net.*;


    server = ServerSocket(11111);
    fprintf(1, horzcat('wait for connection on port 11111 \n'));
    %hostname = getenv('HOSTNAME')
    %home     = getenv('HOME')
    %hostpool = sprintf( '%s/.matlab/local_scheduler_data/R2011b/%s' ,home, hostname)

    run = 1;
    while run
      client = server.accept();
      fprintf(1,horzcat('got connection on port 11111 \n\n'));
      in = BufferedReader(InputStreamReader(client.getInputStream()));
      out = PrintWriter(client.getOutputStream(),true);

      fromClient = in.readLine();
      fromClient = char(fromClient);
      fprintf(1, horzcat('M:(received): ', fromClient, '\n'));

      if (fromClient == 'do_work___') 

        % Set up a custom directory for the PCT
        %hostname = getenv('HOSTNAME');
        home     = getenv('HOME');
        hostpool = sprintf( '%s/.matlab/local_scheduler_data/R2011b/%s' ,home, hostname);
        %system(horzcat('rm -rf ', hostpool, '/*'))
        sch = findResource('scheduler','type','local');
        set(sch, 'DataLocation', hostpool);

        %ret = do_work(rank)
        fh = str2func(funcname);
        ret = fh(split_struct, shared_struct);
        toClient = num2str(ret)

        fprintf(1, horzcat('M: returning result \"', toClient, '\" \n'));
        out.println(toClient);
        fromClient = in.readLine();
        fromClient = char(fromClient);
        fprintf(1, horzcat('M:(received): ', fromClient, '\n'));

        if (fromClient == 'bye_matlab') 
          toClient = 'bye master';
          fprintf(1, 'send: bye master\n');
          out.println(toClient);
          client.close();
          server.close();
          run = 0;
          fprintf(1, 'socket closed\n');
        end
      end

    end

    fprintf(1, '\nAll done.\n')
