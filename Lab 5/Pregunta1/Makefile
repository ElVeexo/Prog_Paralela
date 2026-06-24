runOpenCL:
	g++ main.cpp -o programaOpencl -lOpenCL -std=c++11
	./programaOpencl

runCuda:
	nvcc main.cu -o programaCuda --compiler-options -Wall,-Wextra
	./programaCuda

runCPU:
	g++ mainCPU.cpp -o programaCPU -std=c++11
	./programaCPU

