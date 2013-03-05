import java.lang.*;
import java.io.*;
import java.net.*;

class Master {

  protected Socket sock;
  protected BufferedReader in;
  protected PrintWriter out;

  public Master(){};

  public <T> T[] SendJobsToWorkers(String method, T[] Obj, String s) throws Exception {

    /* Write to disk with FileOutputStream */
    FileOutputStream s_out, f_out;

    /* Write object with ObjectOutputStream */
    ObjectOutputStream obj_out;
    
    int numTasks = Obj.length; 
    String id, shrDataFilePath, dataFilePath;

    SystemCall sys_call = new SystemCall();
    String uid = sys_call.callUID();

    System.out.println(s);

    /* Shared data first */ 
    shrDataFilePath = "/dev/shm/." + uid + "_sh.mat";
    s_out = new FileOutputStream(shrDataFilePath);
    obj_out = new ObjectOutputStream (s_out);
    obj_out.writeObject ( s );

    for ( int i=0; i<numTasks; i= i+1 ) {

      id = Integer.toString(i);
      dataFilePath = "/dev/shm/." + uid + "_sp_" + id + ".mat";
      f_out = new FileOutputStream(dataFilePath);

      /* Write object out to disk */
      obj_out = new ObjectOutputStream (f_out);
      obj_out.writeObject ( Obj[i] );
      
      String mesg = method + ",TMPFS," + id + "," + dataFilePath + ",";
      System.out.println("mesg is: " + mesg);
      out.println(mesg);

    }

    out.println("done");

    String fromMPI, strIdx;
    int idx;


    /* Wait for all the results */
    int jobsAccountedFor = 0;
    
    while ( jobsAccountedFor < numTasks ) {

      try {

        fromMPI = in.readLine();
      
      }  catch (Exception e) {
         continue;
      }

      System.out.println("fromMPI is:" + fromMPI);
      if( fromMPI!= null && !fromMPI.isEmpty() ) {
        jobsAccountedFor = jobsAccountedFor + 1;
        System.out.println ("Got string " + fromMPI);

        /* De-serialize the data file */
        FileInputStream f_in = new FileInputStream (fromMPI);
        ObjectInputStream obj_in = new ObjectInputStream (f_in);
        Object obj = obj_in.readObject();
        
        /* Get job ID from filename */
        String[] splits = fromMPI.split("_");
        System.out.println("this is :" + splits[1].substring(1, splits[1].indexOf(".")));
        strIdx = splits[1].substring(1, splits[1].indexOf("."));
        idx = Integer.parseInt(strIdx);

        Obj[idx] = (T)obj;

      }

    }

    return Obj;

  }


  //public static void main(String[] args) throws Exception {

  public void Launch() throws Exception {

    String fromClient;
    sock = new Socket("localhost", 11112);
    
    System.out.println("Hello there");
    
    in = new BufferedReader(new InputStreamReader (sock.getInputStream()));
    out = new PrintWriter(sock.getOutputStream(), true);
    
  }

  public void Destroy() throws Exception {

    out.println("kill");
    sock.close(); 

  }

}

