#include <iostream>
#include <cstdlib>
#include <ctime>
#include <chrono> 
#include <cuda_runtime.h>


__global__ void kernel(const bool* matriz, int * resultado, int N)
{
    __shared__ float scratch_A[X];
    int global_id = blockIdx.x * blockDim.x + threadIdx.x; 
    int local_id = threadIdx.x;           
    __syncthreads();
    
}

int main() {
    cudaError_t err = cudaSuccess;
    const int N = 1024; 
    size_t tamaño_matriz = sizeof(bool) * N * N;
    
    bool* h_matriz = new  bool[N*N];
    int* h_resultado = new  int[N];

    std::srand(std::time(nullptr));
    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) {
            // Grafo no dirigido: simetría entre (i,j) y (j,i)
            bool conectado = (std::rand() % 100 < 30) ? true : false; // 30% de probabilidad de conexión
            h_matriz[i * N + j] = conectado;
            h_matriz[j * N + i] = conectado;
        }
    }
    
    bool *d_matriz;
    int *d_resultado;

    // Asignar memoria global en la GPU
    err = cudaMalloc(&d_matriz, tamaño_matriz);
    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to allocate device d_matriz (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    err = cudaMalloc(&d_resultado, N * sizeof(int));
    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to allocate device d_resultado (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }    

    err = cudaMemcpy(d_matriz, h_matriz, tamaño_matriz, cudaMemcpyHostToDevice);
    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to copy d_matriz from host to device (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
    
    int threads = 256;
    int blocks = (N + threads - 1) / threads;

    // Calcular el tamaño en bytes de la memoria compartida que necesita CADA bloque
    size_t bytes_memoria_compartida = X;

    auto start = std::chrono::high_resolution_clock::now();

    kernel<<<blocks, threads, bytes_memoria_compartida>>>(d_matriz, d_resultado, N);
    cudaDeviceSynchronize();

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> tiempo_ms = end - start;

    std::cout << "Tiempo de ejecución del Kernel en GPU: " << tiempo_ms.count() << " ms." << std::endl;

    err = cudaMemcpy(h_resultado, d_resultado,  N * sizeof(int), cudaMemcpyDeviceToHost);
    if (err != cudaSuccess)
    {
        fprintf(stderr, "Failed to copy d_resultado from device to host (error code %s)!\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    cudaFree(d_matriz);
    cudaFree(d_resultado);
    delete[] h_matriz;
    delete[] h_resultado;

    return 0;
}