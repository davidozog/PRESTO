import java.lang.*;
import java.io.*;
import java.net.*;

class Worker {

  public static void main(String[] args) throws Exception {

    String myHostname = "";

    if (args.length == 2) {
      try {
        myHostname = args[0];
      } catch (Exception e) {
        System.out.println("Error with input args");
      }
    } else {
        System.out.println("Must have hostname and rank args");
        System.exit(-1);
    }

    String fromClient;

    Socket sock = new Socket(myHostname, 11110);
    
    System.out.println("Hello there");
    
    BufferedReader in = new BufferedReader(new InputStreamReader (sock.getInputStream()));
    PrintWriter out = new PrintWriter(sock.getOutputStream(), true);

    out.println("alive");

    SystemCall sys_call = new SystemCall();
    String uid = sys_call.callUID();

    while ( true ) {

      try {
        fromClient = in.readLine();
      } catch (Exception e) {
        System.out.println ("asdf");
        continue;
      }

      if(fromClient != null && !fromClient.isEmpty() && !fromClient.equals("kill")) {
        System.out.println ("Got string: " + fromClient);

        String[] splits = fromClient.split(",");

        /* De-serialize the shared data file */
        FileInputStream s_in = new FileInputStream ("/dev/shm/."+uid+"_sh.mat");
        ObjectInputStream sobj_in = new ObjectInputStream (s_in);
        Object sobj = sobj_in.readObject();

        /* De-serialize the split data files */
        FileInputStream f_in = new FileInputStream (splits[3]);
        ObjectInputStream obj_in = new ObjectInputStream (f_in);
        Object obj = obj_in.readObject();

        for (int i=0; i<splits.length; i++)
          System.out.println("splits: " + splits[i]);


        /* Reflect the class of the method */
        java.lang.reflect.Method method = null;
        try {

          /* Note: this is for a method with no params: */
          //method = obj.getClass().getMethod(splits[0]);

          /* Note: this is for a method with a single class param: */
          //method = obj.getClass().getMethod(splits[0], obj.getClass());

          /* Note: this is for a method with two class params: */
          Class[] MY_PARAMS = new Class[] {obj.getClass(), sobj.getClass()};
          method = obj.getClass().getMethod(splits[0], MY_PARAMS);

        } catch (SecurityException e) {
            System.out.println("SecurityException");
            System.exit(-1);
        } catch (NoSuchMethodException e) {
            System.out.println("NoSuchMethodException");
            System.exit(-1);
        } 

        Object new_obj = obj.getClass().newInstance();
        //Object new_obj = obj.clone();
        //TestClass new_obj = new TestClass(0,0);

        System.out.println("method name: " + method.getName());

        try {
          /* Note: this is for a method with no params: */
          //method.invoke(obj);

          /* Note: this is for a method with a class param: */
          //method.invoke(new_obj, obj);

          /* Note: this is for a method with a two class params: */
          method.invoke(new_obj, obj, sobj);

        } catch (IllegalArgumentException e) {
            System.out.println("IllegalArgumentException");
            System.exit(-1);

        } catch (IllegalAccessException e) {
            System.out.println("IllegalAccessException");
            System.exit(-1);

        } 

        BarkBeetlePresto B = (BarkBeetlePresto)new_obj;
        
        String r1 = Integer.toString(B.randomSeed);
        String r2 = Float.toString(B.pheromoneAttract);

        System.out.println("results: " + r1 + ", " + r2);

        /* Now write results back into memory */
        FileOutputStream f_out;
        ObjectOutputStream obj_out;
        String id, dataFilePath;
        id = splits[2];
        dataFilePath = "/dev/shm/."+uid+"_r" + id + ".mat";
        f_out = new FileOutputStream(dataFilePath);
        obj_out = new ObjectOutputStream (f_out);
        obj_out.writeObject ( new_obj );

        out.println(dataFilePath);

        System.out.println("Out alive");


      }

      else {
        System.out.println ("Received kill signal");
        sock.close();
        break;
      }
      
    }
    
  }
}


