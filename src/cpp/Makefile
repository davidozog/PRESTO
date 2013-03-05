all:
	g++ -c -o TestClass.o TestClass.cpp
	g++ -c -o Master.o Master.cpp
	g++ -c -o TestApp.o TestApp.cpp
	g++ -o TestApp TestApp.o TestClass.o Master.o

clean:
	rm *.o TestApp
