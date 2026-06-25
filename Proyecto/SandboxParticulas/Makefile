CXX := g++
CC := gcc
NVCC := nvcc
OS := $(shell uname -s)
particle_N := 4096

ifeq ($(findstring MINGW,$(OS)),MINGW)
    # Windows / MSYS2
    NVCC_HOST_CXX ?= $(CXX)
    GL_LIBS := -lglfw3 -lopengl32 -lgdi32 -lpthread
else
    # Linux / WSL
    NVCC_HOST_CXX ?= $(shell if command -v g++-10 >/dev/null 2>&1; then command -v g++-10; else command -v g++; fi)
    GL_LIBS := -lglfw -lGL -ldl -lpthread -lm
endif

PROJECT_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
BUILD_DIR := $(PROJECT_DIR)/build
INCLUDES := -I$(PROJECT_DIR)/include -I$(PROJECT_DIR)/src
PARTICLE_N := $(or $(PARTICLE_COUNT),$(N),$(particle_N))
CONFIG_DEFINES := -DPARTICLE_COUNT_OVERRIDE=$(PARTICLE_N)

CXXFLAGS := -std=c++17 -O2 -Wall -Wextra $(INCLUDES) $(CONFIG_DEFINES)
CFLAGS := -O2 -Wall -Wextra $(INCLUDES) $(CONFIG_DEFINES)
NVCCFLAGS := -std=c++14 -O2 $(INCLUDES) $(CONFIG_DEFINES) -ccbin $(NVCC_HOST_CXX)

.PHONY: all cuda runCPU runCUDA runCPU_N runCUDA_N install-all install-all-ubuntu install-deps-ubuntu install-cuda-ubuntu check-cuda clean

all: $(BUILD_DIR)/sandboxCPU $(BUILD_DIR)/sandboxCUDA

cuda: $(BUILD_DIR)/sandboxCUDA

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/glad.o: $(PROJECT_DIR)/src/glad.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/sandboxCPU: $(PROJECT_DIR)/src/mainCPU.cpp $(BUILD_DIR)/glad.o $(PROJECT_DIR)/include/config.hpp $(PROJECT_DIR)/include/particles.hpp $(PROJECT_DIR)/src/gl_utils.hpp $(PROJECT_DIR)/src/render_utils.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(PROJECT_DIR)/src/mainCPU.cpp $(BUILD_DIR)/glad.o -o $@ $(GL_LIBS)

$(BUILD_DIR)/sandboxCUDA: $(PROJECT_DIR)/src/mainCUDA.cu $(BUILD_DIR)/glad.o $(PROJECT_DIR)/include/config.hpp $(PROJECT_DIR)/include/particles.hpp $(PROJECT_DIR)/src/gl_utils.hpp $(PROJECT_DIR)/src/render_utils.hpp | $(BUILD_DIR)
	$(NVCC) $(NVCCFLAGS) $(PROJECT_DIR)/src/mainCUDA.cu $(BUILD_DIR)/glad.o -o $@ $(GL_LIBS)

runCPU: $(BUILD_DIR)/sandboxCPU
	cd $(PROJECT_DIR) && $(BUILD_DIR)/sandboxCPU

runCUDA: $(BUILD_DIR)/sandboxCUDA
	cd $(PROJECT_DIR) && $(BUILD_DIR)/sandboxCUDA

runCPU_%:
	$(MAKE) -B -C $(PROJECT_DIR) PARTICLE_COUNT=$* $(BUILD_DIR)/sandboxCPU
	cd $(PROJECT_DIR) && $(BUILD_DIR)/sandboxCPU

runCUDA_%:
	$(MAKE) -B -C $(PROJECT_DIR) PARTICLE_COUNT=$* $(BUILD_DIR)/sandboxCUDA
	cd $(PROJECT_DIR) && $(BUILD_DIR)/sandboxCUDA

runCPU_N:
	$(MAKE) -B -C $(PROJECT_DIR) particle_N=$(PARTICLE_N) $(BUILD_DIR)/sandboxCPU
	cd $(PROJECT_DIR) && $(BUILD_DIR)/sandboxCPU

runCUDA_N:
	$(MAKE) -B -C $(PROJECT_DIR) particle_N=$(PARTICLE_N) $(BUILD_DIR)/sandboxCUDA
	cd $(PROJECT_DIR) && $(BUILD_DIR)/sandboxCUDA

install-all: install-all-ubuntu

install-all-ubuntu:
	sudo apt-get update
	sudo apt-get install -y build-essential gcc g++ gcc-10 g++-10 make libglfw3-dev libgl1-mesa-dev mesa-common-dev libglu1-mesa-dev nvidia-cuda-toolkit

install-deps-ubuntu:
	sudo apt-get update
	sudo apt-get install -y build-essential gcc g++ make libglfw3-dev libgl1-mesa-dev mesa-common-dev libglu1-mesa-dev

install-cuda-ubuntu:
	sudo apt-get update
	sudo apt-get install -y gcc-10 g++-10 nvidia-cuda-toolkit

check-cuda:
	nvcc --version
	nvidia-smi

clean:
	rm -rf $(BUILD_DIR)
