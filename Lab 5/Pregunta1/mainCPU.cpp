#include <iostream>
#include <cstdlib>
#include <ctime>
#include <chrono>

int main() {
    const int N = 1024; 

    bool* h_matriz = new bool[N * N];
    int* h_resultado = new int[N];

    for (int i = 0; i < N; i++) {
        h_resultado[i] = 0; 
    }

    std::srand(std::time(nullptr));
    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) {
            // 30% de probabilidad de conexión
            bool conectado = (std::rand() % 100 < 30) ? true : false;
            h_matriz[i * N + j] = conectado;
            h_matriz[j * N + i] = conectado;
        }
    }

    std::cout << "Iniciando cálculo secuencial en CPU..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    // =========================================================================
    // [ALUMNOS] IMPLEMENTAR AQUÍ EL CÁLCULO SECUENCIAL EN CPU
    // Reemplazar la "X" por la lógica de bucles anidados para recorrer el grafo
    // y almacenar el resultado correspondiente para cada fila 'i'.
    // =========================================================================
    
    for (int i = 0; i < N; i++) { // Fijar la fila i
        for (int j = 0; j < N; j++) { // Recorrer las columnas j
            if (i == j) continue;
            for (int k = 0; k < N; k++) { // Dentro de cada columna j recorrer sus valores para buscar los amigos comunes
                if (k == i || k == j) continue;
                if ((h_matriz[i * N + j] == 1) && (h_matriz[i * N + k] == 1) && (h_matriz[j * N + k] == 1)) {
                    h_resultado[i]++;
                }
            }
        }        
    }
    printf("Resultados (Número de amigos comunes para cada fila):\n");
    for (int i = 0; i < N; i++) {
        std::cout << h_resultado[i]/9 << " ";
    }  
    std::cout << std::endl;
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> tiempo_ms = end - start;

    std::cout << "Tiempo de ejecución en CPU: " << tiempo_ms.count() << " ms." << std::endl;

    delete[] h_matriz;
    delete[] h_resultado;

    return 0;
}