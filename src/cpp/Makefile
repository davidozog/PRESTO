all:
	g++ -c -o TestClass.o TestClass.cpp
	g++ -c -o env.o env.cpp
	g++ -c -o Master.o Master.cpp
	g++ -c -o TestApp.o TestApp.cpp
	mpic++ -o cppWorker -I ~/HeadModeling_new/trunk/src/ ~/HeadModeling_new/trunk/objects/omp/HmUtil.o TestClass.o env.o cppWorker.cpp
	g++ -o TestApp TestApp.o TestClass.o Master.o env.o

clean:
	rm *.o TestApp cppWorker
