#pragma once

#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

inline void check_cl(cl_int err, const char* operation) {
    if (err != CL_SUCCESS) {
        std::cerr << "OpenCL error en " << operation << ": " << err << std::endl;
        std::exit(1);
    }
}

inline std::string leer_kernel(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "No se pudo abrir el archivo del kernel: " << path << std::endl;
        std::exit(1);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

inline void print_build_log(cl_program program, cl_device_id device) {
    size_t logSize = 0;
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);

    std::string log(logSize, '\0');
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log.size(), log.data(), nullptr);

    if (!log.empty()) {
        std::cerr << "Build log OpenCL:\n" << log << std::endl;
    }
}

