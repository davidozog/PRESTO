SHELL := /bin/bash

CPP  = mpic++ 
NVCC = nvcc 
CC   = gcc 

CPP_LINK_OMP  = -L/usr/lib64/ -L/usr/lib64/libblas.so.3.0.3 -llapack -fopenmp
CPP_LINK_CUDA = $(CPP_LINK_OMP) -fPic -lcudart -L/usr/local/packages/cuda/4.0/lib64

PRESTO_LINK_OMP = $(CPP_LINK_OMP) -I../../src/cpp -L../../src/cpp -lpresto -lboost_serialization
PRESTO_LINK_CUDA = $(CPP_LINK_CUDA) $(PRESTO_LINK_OMP)

CPP_COMPILE_OMP    = -I $(SOURCE_DIR) -fopenmp -O3
CPP_COMPILE_CUDA   =    $(CPP_COMPILE_OMP) -D CUDA_ENABLED 
CC_COMPILE_OMP     =    $(CPP_COMPILE_OMP)  
CC_COMPILE_CUDA    =    $(CPP_COMPILE_CUDA)
NVCC_COMPILE       = -I $(SOURCE_DIR) -O3   

SOURCE_DIR        = ./src/
CUDA_OBJ_DIR       = ./objects/cuda/
OMP_OBJ_DIR        = ./objects/omp/

FOR_OBJECTS       = Poisson.o PoissonAdi.o PoissonVai.o HeadModel.o  \
		    forward_main.o HmUtil.o

FOR_PRESTO_OBJECTS       = Poisson.o PoissonAdi.o PoissonVai.o HeadModel.o  \
		    presto_forward_main.o HmUtil.o HmTask.o

_FOR_OBJECTS_CUDA = $(FOR_OBJECTS)  solve_adi_cuda_bmd.o  solve_adi_cuda.o poisson_vai_cuda.o reduction.o
_FOR_OBJECTS_OMP  = $(FOR_OBJECTS)  

_PRESTO_FOR_OBJECTS_CUDA = $(FOR_PRESTO_OBJECTS)  solve_adi_cuda_bmd.o  solve_adi_cuda.o poisson_vai_cuda.o reduction.o
_PRESTO_FOR_OBJECTS_OMP  = $(FOR_PRESTO_OBJECTS)  

LFM_OBJECTS       = lfm_main.o Poisson.o PoissonAdi.o LeadField.o HeadModel.o \
                    PoissonVai.o HmUtil.o
_LFM_OBJECTS_CUDA = $(LFM_OBJECTS)  solve_adi_cuda_bmd.o solve_adi_cuda.o   reduction.o poisson_vai_cuda.o
_LFM_OBJECTS_OMP  = $(LFM_OBJECTS)  

COND_OBJECTS       = cond_main.o paral_sa.o cond_class.o CondObjFunc.o Poisson.o \
                     PoissonAdi.o HeadModel.o  current_inj_dat.o simplex.o \
                     PoissonVai.o HmUtil.o OptimMethod.o

_COND_OBJECTS_CUDA = $(COND_OBJECTS)  solve_adi_cuda_bmd.o  solve_adi_cuda.o  reduction.o poisson_vai_cuda.o
_COND_OBJECTS_OMP  = $(COND_OBJECTS)  

FOR_OBJECTS_CUDA  = $(patsubst %, $(CUDA_OBJ_DIR)/%, $(_FOR_OBJECTS_CUDA))
FOR_OBJECTS_OMP   = $(patsubst %, $(OMP_OBJ_DIR)/%, $(_FOR_OBJECTS_OMP))

PRESTO_FOR_OBJECTS_CUDA  = $(patsubst %, $(CUDA_OBJ_DIR)/%, $(_PRESTO_FOR_OBJECTS_CUDA))
PRESTO_FOR_OBJECTS_OMP   = $(patsubst %, $(OMP_OBJ_DIR)/%, $(_PRESTO_FOR_OBJECTS_OMP))

LFM_OBJECTS_CUDA  = $(patsubst %, $(CUDA_OBJ_DIR)/%, $(_LFM_OBJECTS_CUDA))
LFM_OBJECTS_OMP   = $(patsubst %, $(OMP_OBJ_DIR)/%, $(_LFM_OBJECTS_OMP))

COND_OBJECTS_CUDA  = $(patsubst %, $(CUDA_OBJ_DIR)/%, $(_COND_OBJECTS_CUDA))
COND_OBJECTS_OMP   = $(patsubst %, $(OMP_OBJ_DIR)/%, $(_COND_OBJECTS_OMP))

HEADERS            = Poisson.h CondObjFunc.h LeadField.h   HmUtil.h cond_class.h\
                     simplex.h OptimMethod.h paral_sa.h PoissonVai.h HeadModel.h ObjFunc.h HmTask.h

FOR_EXECUTABLE_CUDA = ./bin/for_cuda
FOR_EXECUTABLE_OMP  = ./bin/for_omp

PRESTO_FOR_EXECUTABLE_CUDA = ./bin/presto_for_cuda
PRESTO_FOR_EXECUTABLE_OMP  = ./bin/presto_for_omp

LFM_EXECUTABLE_CUDA = ./bin/lfm_cuda
LFM_EXECUTABLE_OMP  = ./bin/lfm_omp

COND_EXECUTABLE_CUDA = ./bin/cond_cuda
COND_EXECUTABLE_OMP  = ./bin/cond_omp

INCLUDE        = $(SOURCE_DIR)/%.h
 
MAKEFILE = Makefile

.SUFFIXES:
.SUFFIXES: .o .cpp .cu .c

VPATH = $(SOURCE_DIR)

HOSTNAME     := $(shell hostname)

MODULES := 

ifeq ($(HOSTNAME), frankenstein.nic.uoregon.edu)
MODULES := . /usr/local/packages/Modules/current/init/bash;
MODULES := $(MODULES) module purge; module load mpi-pbs/openmpi-1.4.3_gcc-4.3.5-64bit
endif

ifeq ($(HOSTNAME), mist.cs.uoregon.edu)
MODULES := . /usr/local/packages/Modules/current/init/bash;
MODULES := $(MODULES) module purge; module load mpi-pbs/openmpi-1.4.3_gcc-4.3.5-64bit
endif

ifeq ($(HOSTNAME), hn1)
MODULES := source /usr/local/packages/modules/init/bash;
#MODULES := $(MODULES) module purge; module load
MODULES := $(MODULES) module purge; module load mpi-tor/openmpi-1.4.5_gcc-4.4.6
#MODULES := $(MODULES) module purge; module load mpi-tor/openmpi-1.5.4_gcc-4.5.3
endif

ifeq ($(HOSTNAME), vampire.nic.uoregon.edu)
MODULES := $(MODULES) module purge; module load mpi-tor/openmpi-1.4.5_gcc-4.4.6
#MODULES := $(MODULES) module purge; module load mpi-tor/openmpi-1.5.4_gcc-4.5.3
endif

CUDA_MODULES  := $(MODULES) cuda/4.0


all: for_cuda for_omp presto_for_omp presto_for_cuda lfm_cuda lfm_omp cond_cuda cond_omp 
	echo -e 'for_omp: $(MODULES)' > ./cfg/modules.txt
	echo -e 'for_cuda: $(CUDA_MODULES)' >> ./cfg/modules.txt
	echo -e 'lfm_omp: $(MODULES)' >> ./cfg/modules.txt
	echo -e 'lfm_cuda: $(CUDA_MODULES)' >> ./cfg/modules.txt
	echo -e 'cond_omp: $(MODULES)' >> ./cfg/modules.txt
	echo -e 'cond_cuda: $(CUDA_MODULES)' >> ./cfg/modules.txt

for_omp:
	$(MODULES);\
	$(MAKE) $(FOR_EXECUTABLE_OMP)

for_cuda:
	$(CUDA_MODULES);\
	$(MAKE) $(FOR_EXECUTABLE_CUDA)

presto_for_omp:
	$(MODULES);\
	$(MAKE) $(PRESTO_FOR_EXECUTABLE_OMP)

presto_for_cuda:
	$(CUDA_MODULES);\
	$(MAKE) $(PRESTO_FOR_EXECUTABLE_CUDA)

lfm_omp:
	$(MODULES);\
	$(MAKE) $(LFM_EXECUTABLE_OMP)

lfm_cuda:
	$(CUDA_MODULES);\
	$(MAKE) $(LFM_EXECUTABLE_CUDA)

cond_omp:
	$(MODULES);\
	$(MAKE) $(COND_EXECUTABLE_OMP)

cond_cuda:
	$(CUDA_MODULES);\
	$(MAKE) $(COND_EXECUTABLE_CUDA)

$(FOR_EXECUTABLE_OMP):  $(FOR_OBJECTS_OMP) $(MAKEFILE) 
	$(CPP) $(CPP_LINK_OMP) $(FOR_OBJECTS_OMP) -o $@

$(FOR_EXECUTABLE_CUDA):  $(FOR_OBJECTS_CUDA) $(MAKEFILE)
	$(CPP) $(CPP_LINK_CUDA) $(FOR_OBJECTS_CUDA) -o $@

$(PRESTO_FOR_EXECUTABLE_OMP):  $(PRESTO_FOR_OBJECTS_OMP) $(MAKEFILE) 
	$(CPP) $(PRESTO_LINK_OMP) $(PRESTO_FOR_OBJECTS_OMP) -o $@

$(PRESTO_FOR_EXECUTABLE_CUDA):  $(PRESTO_FOR_OBJECTS_CUDA) $(MAKEFILE)
	$(CPP) $(PRESTO_LINK_CUDA) $(PRESTO_FOR_OBJECTS_CUDA) -o $@

$(LFM_EXECUTABLE_OMP):  $(LFM_OBJECTS_OMP) $(MAKEFILE)
	$(CPP) $(CPP_LINK_OMP) $(LFM_OBJECTS_OMP) -o $@

$(LFM_EXECUTABLE_CUDA):  $(LFM_OBJECTS_CUDA) $(MAKEFILE)
	$(CPP) $(CPP_LINK_CUDA) $(LFM_OBJECTS_CUDA) -o $@

$(COND_EXECUTABLE_OMP):  $(COND_OBJECTS_OMP) $(MAKEFILE)
	$(CPP) $(CPP_LINK_OMP) $(COND_OBJECTS_OMP) -o $@

$(COND_EXECUTABLE_CUDA):  $(COND_OBJECTS_CUDA) $(MAKEFILE)
	$(CPP) $(CPP_LINK_CUDA) $(COND_OBJECTS_CUDA) -o $@


$(CUDA_OBJ_DIR)/%.o : $(SOURCE_DIR)/%.cpp $(HEADERS)
	$(CPP) $(CPP_COMPILE_CUDA) -c $< -o $@

$(CUDA_OBJ_DIR)/%.o  : $(SOURCE_DIR)/%.c $(HEADERS)  
	$(CC) $(CC_COMPILE_CUDA) -c $< -o $@

$(CUDA_OBJ_DIR)/%.o : $(SOURCE_DIR)/%.cu $(HEADERS)
	$(NVCC) $(NVCC_COMPILE) -c $< -o $@

$(OMP_OBJ_DIR)/%.o : $(SOURCE_DIR)/%.cpp $(HEADERS) 
	$(CPP) $(CPP_COMPILE_OMP) -c $< -o $@

$(OMP_OBJ_DIR)/%.o  :  $(SOURCE_DIR)/%.c $(HEADERS)
	$(CC) $(CC_COMPILE_OMP) -c $< -o $@


.PHONY: clean cuda

clean:
	rm -f $(LFM_EXECUTABLE_CUDA) $(LFM_EXECUTABLE_OMP)\
	 $(COND_EXECUTABLE_CUDA) \ $(COND_EXECUTABLE_OMP) \
	$(FOR_EXECUTABLE_OMP) $(FOR_EXECUTABLE_CUDA)\
	$(PRESTO_FOR_EXECUTABLE_OMP) $(PRESTO_FOR_EXECUTABLE_CUDA)
	rm -f $(OMP_OBJ_DIR)/*.o $(CUDA_OBJ_DIR)/*.o 
	rm -f *.o *.linkinfo *~
