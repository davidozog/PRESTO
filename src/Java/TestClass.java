public class TestClass implements java.io.Serializable {

  public int a;
  public float b;

  public TestClass(int i, float j) {
    this.a = i;
    this.b = j;
  }

  public TestClass() {
    this.a = 0;
    this.b = 0;
  }

  public TestClass(TestClass another) {
    this.a = another.a;
    this.b = another.b;
  }

  public int getA(TestClass T) {
    return T.a;
  }

  public float getB(TestClass T) {
    return T.b;
  }

  public TestClass TestKernel0(TestClass T1){
    TestClass T2 = new TestClass(T1.a, T1.b);
    T2.a = 7*T2.a;
    T2.b = 7*T2.b;
    return T2;
  }

  public TestClass TestKernel1(int i, float j){
    TestClass T = new TestClass(i, j);
    T.a = 7*i;
    T.b = 7*j;
    return T;
  }

  public TestClass TestKernel2(TestClass T1){
    this.a = 7*T1.a;
    this.b = 7*T1.b;
    return this;
  }

  public TestClass TestKernel3(){
    TestClass T3 = new TestClass(this.a, this.b);
    T3.a = 7*T3.a;
    T3.b = 7*T3.b;
    return T3;
  }

  public static void main(String[] args) throws Exception {
    

    int numTasks = 3;

    Master M = new Master();
    TestClass[] TArrayIn = new TestClass[numTasks];
    TestClass[] TArrayOut = new TestClass[numTasks];

    for (int i=0; i<numTasks; i++) {
      TArrayIn[i] = new TestClass (i, i+numTasks);
    }
    
    M.LaunchMaster();
    TArrayOut = M.SendJobsToWorkers("TestKernel2", TArrayIn);
    
  }

}
