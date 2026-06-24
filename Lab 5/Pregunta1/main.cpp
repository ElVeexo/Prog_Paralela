#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono> 

#define CL_TARGET_OPENCL_VERSION 300 
#include <CL/cl.h> 

// Función auxiliar para leer el archivo del kernel
std::string leer_kernel(const std::string& ruta) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        std::cerr << "No se pudo abrir el archivo del kernel: " << ruta << std::endl;
        exit(1);
    }
    std::stringstream strStream;
    strStream << archivo.rdbuf();
    return strStream.str(); 
}

int main() {
    cl_int err;
    const int N = 1024; 
    size_t tamaño_matriz = sizeof(bool) * N * N;
    
    bool* h_matriz = new bool[N * N];
    bool* h_resultado = new bool[N];

    std::srand(std::time(nullptr));
    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) {
            // Grafo no dirigido: simetría entre (i,j) y (j,i)
            bool conectado = (std::rand() % 100 < 30) ? true : false;
            h_matriz[i * N + j] = conectado;
            h_matriz[j * N + i] = conectado;
        }
    }

    cl_platform_id plataforma;
    cl_device_id dispositivo;

    clGetPlatformIDs(1, &plataforma, NULL);
    err = clGetDeviceIDs(plataforma, CL_DEVICE_TYPE_GPU, 1, &dispositivo, NULL);
    if (err != CL_SUCCESS) {
        clGetDeviceIDs(plataforma, CL_DEVICE_TYPE_CPU, 1, &dispositivo, NULL);
    }

    cl_context contexto = clCreateContext(NULL, 1, &dispositivo, NULL, NULL, &err);
    cl_command_queue cola = clCreateCommandQueueWithProperties(contexto, dispositivo, NULL, &err);

    // Cargar y construir el programa desde el archivo externo "kernel.cl"
    std::string codigo_fuente = leer_kernel("kernel.cl");
    const char* src = codigo_fuente.c_str();
    size_t src_size = codigo_fuente.length();

    cl_program programa = clCreateProgramWithSource(contexto, 1, &src, &src_size, &err);
    err = clBuildProgram(programa, 1, &dispositivo, NULL, NULL, NULL);
    
    if (err != CL_SUCCESS) {
        char log[2048];
        clGetProgramBuildInfo(programa, dispositivo, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
        std::cerr << "Error de compilación del Kernel OpenCL:\n" << log << std::endl;
        return 1;
    }

    // Crear el objeto Kernel (Debe coincidir con el nombre dentro de kernel.cl)
    cl_kernel kernel = clCreateKernel(programa, "kernel_grafos", &err);
    
    cl_mem d_matriz = clCreateBuffer(contexto, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, tamaño_matriz, h_matriz, &err);
    cl_mem d_resultado = clCreateBuffer(contexto, CL_MEM_WRITE_ONLY, sizeof(bool) * N, NULL, &err);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_matriz);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_resultado);
    clSetKernelArg(kernel, 2, sizeof(int), &N);
    
    // Argumento para la Memoria Local (__local). Reemplazar X por los bytes correctos.
    // Recuerden que pasar NULL como último parámetro reserva memoria local dinámica.
    clSetKernelArg(kernel, 3, X, NULL); 

    int threads = 256;
    

    size_t global_size = ((N + threads - 1) / threads) * threads; 
    size_t local_size = threads;

    auto start = std::chrono::high_resolution_clock::now();

    clEnqueueNDRangeKernel(cola, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);

    // OBLIGATORIO en OpenCL: Bloquear la CPU hasta que los comandos de la cola terminen
    clFinish(cola);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> tiempo_ms = end - start;

    std::cout << "Tiempo de ejecución del Kernel en OpenCL: " << tiempo_ms.count() << " ms." << std::endl;

    clEnqueueReadBuffer(cola, d_resultado, CL_TRUE, 0, sizeof(bool) * N, h_resultado, 0, NULL, NULL);

    clReleaseMemObject(d_matriz); 
    clReleaseMemObject(d_resultado);
    clReleaseKernel(kernel); 
    clReleaseProgram(programa);
    clReleaseCommandQueue(cola); 
    clReleaseContext(contexto);
    
    delete[] h_matriz;
    delete[] h_resultado;

    return 0;
}