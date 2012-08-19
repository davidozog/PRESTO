all:
	mex mat2shmem.c  -lrt
	mex mat2shmemQ.c  -lrt
	mex shmem2mat.c  -lrt
	mex shmResult2mat.c  -lrt

clean:
	rm mat2shmem.mexa64 shmem2mat.mexa64 shmResult2mat.mexa64 mat2shmemQ.mexa64
