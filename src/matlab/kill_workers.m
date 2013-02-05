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

    out.println('kill');


