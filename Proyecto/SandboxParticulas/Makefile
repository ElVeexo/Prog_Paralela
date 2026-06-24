CXX := g++
CC := gcc
NVCC := nvcc

CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude -Isrc
CFLAGS := -O2 -Wall -Wextra -Iinclude
NVCCFLAGS := -std=c++17 -O2 -Iinclude -Isrc
GL_LIBS := -lglfw -lGL -ldl -lpthread -lm
CL_LIBS := -lOpenCL

BUILD_DIR := build

.PHONY: all all3 cuda runCPU runGPU runCUDA install-deps-ubuntu install-cuda-ubuntu check-opencl check-cuda clean

all: $(BUILD_DIR)/sandboxCPU $(BUILD_DIR)/sandboxGPU

all3: all $(BUILD_DIR)/sandboxCUDA

cuda: $(BUILD_DIR)/sandboxCUDA

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/glad.o: src/glad.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/sandboxCPU: src/mainCPU.cpp $(BUILD_DIR)/glad.o include/config.hpp include/particles.hpp src/gl_utils.hpp src/render_utils.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) src/mainCPU.cpp $(BUILD_DIR)/glad.o -o $@ $(GL_LIBS)

$(BUILD_DIR)/sandboxGPU: src/main.cpp $(BUILD_DIR)/glad.o include/config.hpp include/particles.hpp src/gl_utils.hpp src/render_utils.hpp src/opencl_utils.hpp kernels/kernel.cl | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) src/main.cpp $(BUILD_DIR)/glad.o -o $@ $(GL_LIBS) $(CL_LIBS)

$(BUILD_DIR)/sandboxCUDA: src/mainCUDA.cu $(BUILD_DIR)/glad.o include/config.hpp include/particles.hpp src/gl_utils.hpp src/render_utils.hpp | $(BUILD_DIR)
	$(NVCC) $(NVCCFLAGS) src/mainCUDA.cu $(BUILD_DIR)/glad.o -o $@ $(GL_LIBS)

runCPU: $(BUILD_DIR)/sandboxCPU
	./$(BUILD_DIR)/sandboxCPU

runGPU: $(BUILD_DIR)/sandboxGPU
	./$(BUILD_DIR)/sandboxGPU

runCUDA: $(BUILD_DIR)/sandboxCUDA
	./$(BUILD_DIR)/sandboxCUDA

install-deps-ubuntu:
	sudo apt-get update
	sudo apt-get install -y build-essential gcc g++ make libglfw3-dev libgl1-mesa-dev mesa-common-dev libglu1-mesa-dev ocl-icd-opencl-dev opencl-headers clinfo

install-cuda-ubuntu:
	sudo apt-get update
	sudo apt-get install -y nvidia-cuda-toolkit

check-opencl:
	clinfo

check-cuda:
	nvcc --version
	nvidia-smi

clean:
	rm -rf $(BUILD_DIR)
