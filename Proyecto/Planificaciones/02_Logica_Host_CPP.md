# Logica Host C++ para Version GPU

## Archivo objetivo

`src/main.cpp` implementa:

```text
OpenCL tiled O(N^2) + __local + OpenGL 1280x720
```

Este ejecutable es independiente de `mainCPU.cpp`. No hay tecla para cambiar CPU/GPU.

## Responsabilidades de `main.cpp`

1. Incluir `config.hpp` y `particles.hpp`.
2. Crear ventana OpenGL de `1280x720`.
3. Crear VBO/VAO para `RenderParticle`.
4. Inicializar OpenCL.
5. Crear buffers:
   - `d_particlesIn`;
   - `d_particlesOut`;
   - `d_renderParticles`.
6. Compilar `kernels/kernel.cl`.
7. Ejecutar kernel OpenCL tiled en cada frame.
8. Leer `RenderParticle` desde GPU.
9. Actualizar VBO con `glBufferSubData`.
10. Renderizar con `GL_POINTS`.
11. Medir FPS y frame time con `<chrono>`.

## Inicializacion

Partir del estilo de `Lab 5/Pregunta1/main.cpp`:

- `leer_kernel`;
- `clGetPlatformIDs`;
- `clGetDeviceIDs`;
- `clCreateContext`;
- `clCreateCommandQueueWithProperties`;
- `clCreateProgramWithSource`;
- `clBuildProgram`;
- `clCreateKernel`;
- `clCreateBuffer`.

Partir del estilo de `Lab 5/Pregunta2/example.cpp`:

- `glfwInit`;
- `glfwCreateWindow`;
- `gladLoadGLLoader`;
- `createShaderProgram`;
- `glGenVertexArrays`;
- `glGenBuffers`;
- ciclo `while (!glfwWindowShouldClose(window))`.

## Ventana OpenGL

Usar siempre `Config`:

```cpp
GLFWwindow* window = glfwCreateWindow(
    Config::WINDOW_WIDTH,
    Config::WINDOW_HEIGHT,
    "Sandbox GPU OpenCL",
    NULL,
    NULL
);
```

Valores iniciales:

```text
1280x720
```

## Buffers OpenCL

```cpp
std::vector<Particle> particles =
    crear_particulas(Config::PARTICLE_COUNT, 1234);

std::vector<RenderParticle> renderParticles(Config::PARTICLE_COUNT);

cl_mem d_particlesIn = clCreateBuffer(
    contexto,
    CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
    sizeof(Particle) * Config::PARTICLE_COUNT,
    particles.data(),
    &err
);

cl_mem d_particlesOut = clCreateBuffer(
    contexto,
    CL_MEM_READ_WRITE,
    sizeof(Particle) * Config::PARTICLE_COUNT,
    nullptr,
    &err
);

cl_mem d_renderParticles = clCreateBuffer(
    contexto,
    CL_MEM_WRITE_ONLY,
    sizeof(RenderParticle) * Config::PARTICLE_COUNT,
    nullptr,
    &err
);
```

## Kernels necesarios

Solo dos kernels:

```cpp
cl_kernel kUpdate = clCreateKernel(programa, "update_particles_tiled", &err);
cl_kernel kBuildRender = clCreateKernel(programa, "build_render_particles", &err);
```

No hay grilla espacial ni prefix sum. La optimizacion obligatoria es el tiling con memoria local `__local`.

## Build options para OpenCL

Pasar constantes desde `config.hpp`:

```cpp
std::string buildOptions =
    " -D PARTICLE_COUNT=" + std::to_string(Config::PARTICLE_COUNT) +
    " -D TILE_SIZE=" + std::to_string(Config::LOCAL_SIZE) +
    " -D WORLD_MIN_X=" + std::to_string(Config::WORLD_MIN_X) + "f" +
    " -D WORLD_MAX_X=" + std::to_string(Config::WORLD_MAX_X) + "f" +
    " -D WORLD_MIN_Y=" + std::to_string(Config::WORLD_MIN_Y) + "f" +
    " -D WORLD_MAX_Y=" + std::to_string(Config::WORLD_MAX_Y) + "f" +
    " -D WALL_BOUNCE=" + std::to_string(Config::WALL_BOUNCE) + "f";
```

## Ciclo principal GPU

```cpp
while (!glfwWindowShouldClose(window)) {
    auto frameStart = std::chrono::high_resolution_clock::now();

    processInput(window);

    if (reiniciar) {
        particles = crear_particulas(Config::PARTICLE_COUNT, seed++);
        clEnqueueWriteBuffer(cola, d_particlesIn, CL_TRUE, 0,
            sizeof(Particle) * Config::PARTICLE_COUNT,
            particles.data(), 0, NULL, NULL);
    }

    if (!paused) {
        clSetKernelArg(kUpdate, 0, sizeof(cl_mem), &d_particlesIn);
        clSetKernelArg(kUpdate, 1, sizeof(cl_mem), &d_particlesOut);
        clSetKernelArg(kUpdate, 2, sizeof(float), &dt);
        clSetKernelArg(kUpdate, 3, sizeof(Particle) * Config::LOCAL_SIZE, NULL);

        clEnqueueNDRangeKernel(cola, kUpdate, 1, NULL,
            &globalSize, &localSize, 0, NULL, NULL);

        std::swap(d_particlesIn, d_particlesOut);
    }

    clSetKernelArg(kBuildRender, 0, sizeof(cl_mem), &d_particlesIn);
    clSetKernelArg(kBuildRender, 1, sizeof(cl_mem), &d_renderParticles);

    clEnqueueNDRangeKernel(cola, kBuildRender, 1, NULL,
        &globalSize, &localSize, 0, NULL, NULL);

    clEnqueueReadBuffer(cola, d_renderParticles, CL_TRUE, 0,
        sizeof(RenderParticle) * Config::PARTICLE_COUNT,
        renderParticles.data(), 0, NULL, NULL);

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

## Interactividad minima

- `SPACE`: pausar/reanudar.
- `R`: reiniciar particulas.
- `1`: seleccionar emision roja.
- `2`: seleccionar emision verde.
- `3`: seleccionar emision azul.

La emision puede ser minima: al presionar mouse o una tecla, reemplazar un bloque pequeno de particulas alrededor del cursor y copiar ese bloque a GPU con `clEnqueueWriteBuffer`.

## Medicion

Medir frame completo, porque interesa la demo visual:

```cpp
auto frameStart = std::chrono::high_resolution_clock::now();
// input + update OpenCL + read buffer + VBO + render
auto frameEnd = std::chrono::high_resolution_clock::now();
```

Reportar cada 60 frames:

```text
OpenCL tiled | N=8192 | frame=12.4 ms | FPS=80.6
```
