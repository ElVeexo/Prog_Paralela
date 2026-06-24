# CPU Secuencial y Mediciones

## Archivo objetivo

`src/mainCPU.cpp` implementa:

```text
CPU O(N^2) + OpenGL 1280x720
```

No es un programa de consola solamente. Debe tener ventana y render igual que la version GPU.

## Responsabilidades de `mainCPU.cpp`

1. Incluir `config.hpp` y `particles.hpp`.
2. Crear particulas con la misma funcion que usa GPU.
3. Crear ventana OpenGL.
4. Crear VBO/VAO de `RenderParticle`.
5. Ejecutar update secuencial `O(N^2)` en CPU.
6. Convertir `Particle` a `RenderParticle`.
7. Actualizar VBO con `glBufferSubData`.
8. Renderizar con `GL_POINTS`.
9. Medir FPS y frame time con `<chrono>`.

## Update CPU naive

```cpp
void update_cpu_naive(
    const std::vector<Particle>& in,
    std::vector<Particle>& out,
    float dt
) {
    const int n = static_cast<int>(in.size());

    for (int i = 0; i < n; ++i) {
        Particle p = in[i];

        for (int j = 0; j < n; ++j) {
            if (i == j) continue;
            aplicar_interaccion_cpu(p, in[j]);
        }

        integrar_y_rebotar_cpu(p, dt);
        out[i] = p;
    }
}
```

Al final de cada frame:

```cpp
std::swap(particlesIn, particlesOut);
```

Esto replica el doble buffer de GPU y hace la comparacion mas justa.

## Ciclo principal CPU

```cpp
while (!glfwWindowShouldClose(window)) {
    auto frameStart = std::chrono::high_resolution_clock::now();

    processInput(window);

    if (reiniciar) {
        particlesIn = crear_particulas(Config::PARTICLE_COUNT, seed++);
    }

    if (!paused) {
        update_cpu_naive(particlesIn, particlesOut, dt);
        std::swap(particlesIn, particlesOut);
    }

    build_render_particles_cpu(particlesIn, renderParticles);

    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        sizeof(RenderParticle) * Config::PARTICLE_COUNT,
        renderParticles.data());

    render_particles(shaderProgram, VAO);

    glfwSwapBuffers(window);
    glfwPollEvents();

    medir_frame(frameStart);
}
```

## Medicion

Se mide el frame completo:

```text
input + simulacion + conversion visual + VBO + render
```

Esto es intencional, porque el objetivo de la demo es comparar rendimiento visual.

Salida sugerida cada 60 frames:

```text
CPU    | N=4096 | frame=38.2 ms | FPS=26.1
OpenCL tiled | N=4096 | frame=11.7 ms | FPS=85.4
CUDA tiled   | N=4096 | frame=10.9 ms | FPS=91.7
```

## Tabla final

Probar:

```text
N = 1024, 4096, 8192, 16384
```

Tabla:

```text
N       CPU FPS   CPU ms   OpenCL FPS   OpenCL ms   CUDA FPS   CUDA ms
1024    ...       ...      ...          ...         ...        ...
4096    ...       ...      ...          ...         ...        ...
8192    ...       ...      ...          ...         ...        ...
16384   ...       ...      ...          ...         ...        ...
```

## Makefile recomendado

```makefile
CXX=g++
CXXFLAGS=-std=c++17 -O2 -Wall -Wextra -Iinclude
GL_LIBS=-lglfw -lGL -ldl -lpthread -lm
CL_LIBS=-lOpenCL

runCPU:
	$(CXX) $(CXXFLAGS) src/mainCPU.cpp src/glad.c -o sandboxCPU $(GL_LIBS)
	./sandboxCPU

runGPU:
	$(CXX) $(CXXFLAGS) src/main.cpp src/glad.c -o sandboxGPU $(GL_LIBS) $(CL_LIBS)
	./sandboxGPU

runCUDA:
	nvcc -std=c++17 -O2 -Iinclude -Isrc src/mainCUDA.cu src/glad.c -o sandboxCUDA $(GL_LIBS)
	./sandboxCUDA
```

## Que defender

- Ambas versiones tienen la misma ventana y el mismo render.
- Ambas usan la misma cantidad de particulas y reglas.
- CPU calcula interacciones `O(N^2)` secuencialmente.
- OpenCL calcula interacciones `O(N^2)` en paralelo con un work-item por particula, cargando tiles a `__local` y sincronizando con `barrier`.
- CUDA calcula interacciones `O(N^2)` en paralelo con un hilo por particula, cargando tiles a `__shared__` y sincronizando con `__syncthreads`.
- OpenCL/CUDA aun pagan costo de lectura del buffer visual y actualizacion de VBO.
- La comparacion visual se resume en FPS y frame time.
