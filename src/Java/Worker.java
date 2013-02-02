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

    while ( true ) {

      try {
        fromClient = in.readLine();
      } catch (Exception e) {
        continue;
      }

      if(fromClient != null && !fromClient.isEmpty()) {
        System.out.println ("Got string: " + fromClient);

        String[] splits = fromClient.split(",");

        /* De-serialize the data file */
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

          /* Note: this is for a method with a class param: */
          method = obj.getClass().getMethod(splits[0], obj.getClass());

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
          method.invoke(new_obj, obj);

        } catch (IllegalArgumentException e) {
            System.out.println("IllegalArgumentException");
            System.exit(-1);

        } catch (IllegalAccessException e) {
            System.out.println("IllegalAccessException");
            System.exit(-1);

        } 

        System.out.println("Out alive");

        TestClass T = (TestClass)new_obj;
        
        String r1 = Integer.toString(T.a);
        String r2 = Float.toString(T.b);

        System.out.println("results: " + r1 + ", " + r2);

      }
      
    }
    

//        for ( int i=0; i<3; i= i+1 ) {
//        
//            out.println("Rinth_testfunc, TMPFS, " + Integer.toString(i) + ", /dev/shm/.jd_7.mat,");
//        
//        }
//
//        out.println("done");
//
//        String myin = in.readLine();
//
//        System.out.println ("Got string " + myin);


  }
}


