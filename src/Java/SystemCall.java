import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

public class SystemCall {

    public String callUID() {

        Runtime r = Runtime.getRuntime();
        String uid = "";

        try {

            Process p = r.exec("id");
            InputStream in = p.getInputStream();
            BufferedInputStream buf = new BufferedInputStream(in);
            InputStreamReader inread = new InputStreamReader(buf);
            BufferedReader bufferedreader = new BufferedReader(inread);

            String line;
            while ((line = bufferedreader.readLine()) != null) {
                String[] lineSplit = line.split(" ");
                uid = lineSplit[0];
                int lft = uid.indexOf('=')+1;
                int rht = uid.indexOf('(');
                uid = uid.substring(lft, rht);
                //System.out.println(uid);
            }

            try {
                if (p.waitFor() != 0) {
                    System.err.println("exit value = " + p.exitValue());
                }
            } catch (InterruptedException e) {
                System.err.println(e);
            } finally {

                bufferedreader.close();
                inread.close();
                buf.close();
                in.close();
            }
        } catch (IOException e) {
            System.err.println(e.getMessage());
        }

        return uid;

    }

    public String system(String cmd) {

        Runtime r = Runtime.getRuntime();
        String pid = "";

        try {

            Process p = r.exec(cmd);
            InputStream in = p.getInputStream();
            BufferedInputStream buf = new BufferedInputStream(in);
            InputStreamReader inread = new InputStreamReader(buf);
            BufferedReader bufferedreader = new BufferedReader(inread);

            String line;
            while ((line = bufferedreader.readLine()) != null) {
                System.out.println(line);
            }

            try {
                if (p.waitFor() != 0) {
                    System.err.println("exit value = " + p.exitValue());
                }
            } catch (InterruptedException e) {
                System.err.println(e);
            } finally {

                bufferedreader.close();
                inread.close();
                buf.close();
                in.close();
            }
        } catch (IOException e) {
            System.err.println(e.getMessage());
        }

      return pid;

    }

}

