DEP = bcnrand.cu bcnrand.h  

bcnrand:	$(DEP) 		
	nvcc -O3 -gencode arch=compute_20,code=sm_20  bcnrand.cu -o bcnrand
