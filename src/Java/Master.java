import java.lang.*;
import java.io.*;
import java.net.*;

class Master {

  protected Socket sock;
  protected BufferedReader in;
  protected PrintWriter out;

  public Master(){};

  public TestClass[] SendJobsToWorkers(String method, TestClass[] T) throws Exception {

    System.out.println("Remote method is " + method);

    System.out.println("array length is " + Integer.toString(T.length));

    /* Write to disk with FileOutputStream */
    FileOutputStream f_out;

    /* Write object with ObjectOutputStream */
    ObjectOutputStream obj_out;
    
    int numTasks = T.length; 
    String id, dataFilePath;

    for ( int i=0; i<3; i= i+1 ) {

      id = Integer.toString(i);
      dataFilePath = "/dev/shm/.jv_" + id + ".data";
      f_out = new FileOutputStream(dataFilePath);

      /* Write object out to disk */
      obj_out = new ObjectOutputStream (f_out);
      obj_out.writeObject ( T[i] );
      
      String mesg = method + ",TMPFS," + id + "," + dataFilePath + ",";
      System.out.println("mesg is: " + mesg);
      out.println(mesg);

    }

    out.println("done");

    String myin = in.readLine();
    System.out.println ("Got string " + myin);

    return T;

  }


  //public static void main(String[] args) throws Exception {

  public void LaunchMaster() throws Exception {

    String fromClient;
    sock = new Socket("localhost", 11112);
    
    System.out.println("Hello there");
    
    in = new BufferedReader(new InputStreamReader (sock.getInputStream()));
    out = new PrintWriter(sock.getOutputStream(), true);
    
  }

}

