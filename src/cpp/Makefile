CPP_LINK_OMP  = -L/usr/lib64/ -L/usr/lib64/libblas.so.3.0.3 -llapack -fopenmp
CPP_LINK_CUDA = $(CPP_LINK_OMP) -fPIC -lcudart -L/usr/local/packages/cuda/4.0/lib64

FOR_PRESTO_OBJECTS = Poisson.o PoissonAdi.o PoissonVai.o HeadModel.o HmUtil.o HmTask.o
_PRESTO_FOR_OBJECTS_CUDA = $(FOR_PRESTO_OBJECTS)  solve_adi_cuda.o solve_adi_cuda_bmd.o poisson_vai_cuda.o reduction.o                                                                                                                                                        
HM_DIR = ~/Repos/PRESTO/examples/HeadModeling_r21/
HM_OBJ_DIR = $(HM_DIR)/objects/cuda/
PRESTO_FOR_OBJECTS_CUDA  = $(patsubst %, $(HM_OBJ_DIR)/%, $(_PRESTO_FOR_OBJECTS_CUDA))

HM_OBJECTS = $(PRESTO_FOR_OBJECTS_CUDA)


all:
	g++ -Wall -fPIC -c -o TestClass.o TestClass.cpp
	g++ -Wall -fPIC -c -o env.o env.cpp
	g++ -Wall -fPIC -c -o Master.o Master.cpp
	g++ -c -o TestApp.o TestApp.cpp
	#g++ -c -o HeadModelForward.o HeadModelForward.cpp
	#
	# For HmForward:
	mpic++ -lboost_serialization $(CPP_LINK_CUDA) -o cppWorker -I $(HM_DIR)/src/ $(HM_OBJECTS) TestClass.o env.o cppWorker.cpp
	#
	# For TestClass:
	#mpic++ -o cppWorker -I ~/HeadModeling_new/trunk/src/ ~/HeadModeling_new/trunk/objects/omp/HmUtil.o TestClass.o env.o cppWorker.cpp
	g++ -shared -Wl,-soname,libpresto.so.1 -o libpresto.so.1.0 Master.o env.o TestClass.o
	g++ -I. -L. -lboost_serialization -lpresto -o TestApp TestApp.o TestClass.o
	#g++ -o HeadModelApp HeadModelForward.o Master.o env.o

clean:
	rm *.o TestApp cppWorker
