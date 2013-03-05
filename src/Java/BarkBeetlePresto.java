import java.io.*;

public class BarkBeetlePresto implements java.io.Serializable {

  public int randomSeed; 
  public int longDistance; 
  public float pheromoneAttract;
  public String windDirection;
  public float resistanceValue;
  public int beetleNeighborhoodSearch; 
  public int cellBeetles;
  public int gridWidth; 
  public int standardDeviationCellBeetles; 
  public String cellData;
  public String tempData;
  public int simLength; 
  public int gridHeight;
  public int survivalMethod; 
  public float mortality;
  public String resistance;
  public float survival;
  public String nearestNeighbor;
  public int initialCells;

  public String shared_inputs;

  public BarkBeetlePresto(int rand, float phA, float rstV, float mrt, int longD) {
    /* split */
    this.randomSeed = rand; 
    this.longDistance = longD; 
    this.pheromoneAttract = phA;
    this.resistanceValue = rstV;
    this.mortality = mrt;

    /* shared */
    this.windDirection = "Northeast";
    this.beetleNeighborhoodSearch = 40; 
    this.cellBeetles = 10;
    this.gridWidth = 100; 
    this.standardDeviationCellBeetles = 0; 
    this.cellData = "../data/study_site.csv";
    this.tempData = "../data";
    this.simLength = 20; 
    this.gridHeight = 100;
    this.survivalMethod = 0; 
    this.resistance = "../data/resistance.csv";
    this.survival = 0.0f;
    this.nearestNeighbor = "../data/nearestNeighborDistances.csv";
    this.initialCells = 100;
  }

  public BarkBeetlePresto() {
    /* split */
    this.randomSeed = 1; 
    this.longDistance = 20; 
    this.pheromoneAttract = 0.1f;
    this.resistanceValue = 0.1f;
    this.mortality = 0.1f;

    /* shared */
    this.windDirection = "Northeast";
    this.beetleNeighborhoodSearch = 40; 
    this.cellBeetles = 10;
    this.gridWidth = 100; 
    this.standardDeviationCellBeetles = 0;
    this.cellData = "../data/study_site.csv";
    this.tempData = "../data";
    this.simLength = 20; 
    this.gridHeight = 100;
    this.survivalMethod = 0; 
    this.resistance = "../data/resistance.csv";
    this.survival = 0.0f;
    this.nearestNeighbor = "../data/nearestNeighborDistances.csv";
    this.initialCells = 100;
  }


  public BarkBeetlePresto(BarkBeetlePresto another) {
    this.randomSeed = another.randomSeed;
    this.pheromoneAttract = another.pheromoneAttract;
  }

  public int getRandomSeed(BarkBeetlePresto B) {
    return B.randomSeed;
  }

  public float getLongDistance(BarkBeetlePresto B) {
    return B.longDistance;
  }

  public String genInputArg() {
    String inarg = "";

    inarg += this.randomSeed + "\trandomSeed\t" + this.randomSeed +
             ",longDistance\t" + this.longDistance +
             ",pheromoneAttract\t" + this.pheromoneAttract +
             ",windDirection\t" + this.windDirection +
             ",resistanceValue\t" + this.resistanceValue +
             ",beetleNeighborhoodSearch\t" + this.beetleNeighborhoodSearch +
             ",cellBeetles\t" + this.cellBeetles +
             ",gridWidth\t" + this.gridWidth +
             ",standardDeviationCellBeetles\t" + this.standardDeviationCellBeetles +
             ",cellData\t" + this.cellData +
             ",tempData\t" + this.tempData +
             ",simLength\t" + this.simLength +
             ",gridHeight\t" + this.gridHeight +
             ",survivalMethod\t" + this.survivalMethod +
             ",mortality\t" + this.mortality +
             ",resistance\t" + this.resistance +
             ",survival\t" + this.survival +
             ",nearestNeighbor\t" + this.nearestNeighbor +
             ",initialCells\t" + this.initialCells + "\n";

    return inarg;
  }


  public void FileWrite(String to_write) {
    try {
      FileWriter fstream = new FileWriter("paramfiles/params_"+this.randomSeed+".txt");
      BufferedWriter out = new BufferedWriter(fstream);
      out.write(to_write);
      out.close();
    }
    catch (Exception e) {
      System.err.println("Error: " + e.getMessage());
    }
  }


  public BarkBeetlePresto TestKernel2(BarkBeetlePresto B, String s){
    int id = B.randomSeed;
    this.randomSeed = B.randomSeed; 
    this.longDistance = B.longDistance; 
    this.pheromoneAttract = B.pheromoneAttract;
    this.resistanceValue = B.resistanceValue;
    this.mortality = B.mortality;

    String inputArg = B.genInputArg();

    B.FileWrite(inputArg);

    String geo_path = "/home11/ozog/Repos/PRESTO/test/ozog_geo/";

      String java_call = "/usr/bin/java -Xmx512m -cp \"" + geo_path + "lib/*\" " +
                         "repast.simphony.batch/InstanceRunner " + geo_path + "/scenario.rs/batchparams.xml " +
                          geo_path + "/scenario.rs " + inputArg;
  
  
      SystemCall sys_call = new SystemCall();
      java_call = "sed -n 1,1p /home11/ozog/myoutty.txt";
      java_call = "sh presto_script.sh " + id + " paramfiles/params_"+id+".txt";
      System.out.println("javacall: " + java_call);
      String out = sys_call.system(java_call);
  
      System.out.println("out is:" + out);
  
    /* this is so stupid */
    while (true) {
      File f = new File("instance_"+id+"/output/ModelOutput1.dbf");
      if(f.exists()) { break; }
    }

    return this;
  }


  public static void main(String[] args) throws Exception {

    int numTasks = 8;

    Master M = new Master();
    BarkBeetlePresto[] BArrayIn = new BarkBeetlePresto[numTasks];
    BarkBeetlePresto[] BArrayOut = new BarkBeetlePresto[numTasks];

    for (int i=0; i<numTasks; i++) {
    //BArrayIn[i] = new BarkBeetlePresto(int rand, float phA, float rstV, float mrt, float longD);
      BArrayIn[i] = new BarkBeetlePresto(i+1, 0.1f, 0.1f, 0.1f, 20);
    }

    String BShared = "hi.";
    
    M.Launch();
    BArrayOut = M.SendJobsToWorkers("TestKernel2", BArrayIn, BShared);

    for (int i=0; i<numTasks; i++) {
      System.out.println("Final Results #"+Integer.toString(i) +" are: " + 
      Integer.toString(BArrayOut[i].randomSeed) + " " + Float.toString(BArrayOut[i].pheromoneAttract));
    }

    M.Destroy();
    
  }

}
