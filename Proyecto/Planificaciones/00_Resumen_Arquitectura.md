# Resumen de Arquitectura

## Objetivo real del proyecto

Implementar un sandbox 2D de particulas con tres ejecutables comparables:

```text
sandboxCPU        # simulacion O(N^2) en CPU + ventana OpenGL
sandboxCUDA       # simulacion tiled O(N^2) en CUDA + __shared__ + ventana OpenGL
sandboxGPU        # simulacion tiled O(N^2) en OpenCL + __local + ventana OpenGL
```

Los tres deben renderizar en tiempo real con OpenGL. La comparacion principal no sera un benchmark ciego por consola, sino una demo visual donde se comparan:

- FPS.
- tiempo de frame.
- comportamiento de la simulacion.
- escalabilidad al cambiar `N`.

## Alcance del nucleo

Lo unico que se debe implementar:

- `mainCPU.cpp`: simulacion secuencial `O(N^2)` en CPU, ventana OpenGL, render con `GL_POINTS`, medicion de FPS y frame time con `<chrono>`.
- `mainCUDA.cu`: simulacion `O(N^2)` en GPU usando CUDA con tiling y memoria compartida `__shared__`, ventana OpenGL, render con `GL_POINTS`.
- `main.cpp`: simulacion `O(N^2)` en GPU usando OpenCL con tiling, memoria local `__local` y `barrier(CLK_LOCAL_MEM_FENCE)`, ventana OpenGL, render con `GL_POINTS`.
- `config.hpp`: archivo unico de configuracion compartido por las tres versiones.
- `particles.hpp`: structs compartidos y funciones comunes de inicializacion/reglas.
- `kernel.cl`: kernel OpenCL tiled para actualizar particulas.
- Shaders simples para dibujar particulas circulares con colores por material.

## Fuera de scope

No forman parte del nucleo:

- Grilla espacial.
- Prefix sum paralelo.
- Interop OpenCL/OpenGL con VBO compartido.
- Optimizaciones adicionales mas alla del tiling obligatorio.

Se pueden mencionar como trabajo futuro, pero no deben aparecer como fases necesarias del plan.

## Estructura minima recomendada

```text
Proyecto/SandboxParticulas/
├── Makefile
├── README.md
├── include/
│   ├── config.hpp
│   ├── particles.hpp
│   └── glad.h
├── src/
│   ├── mainCPU.cpp
│   ├── main.cpp
│   ├── mainCUDA.cu
│   ├── glad.c
│   ├── gl_utils.hpp
│   └── opencl_utils.hpp
├── kernels/
│   └── kernel.cl
└── shaders/
    ├── particles.vert
    └── particles.frag
```

## Flujo de ejecucion CPU

```text
inicializar config
crear particulas
crear ventana OpenGL 1280x720
crear VBO de RenderParticle
mientras ventana abierta:
    procesar input
    si no esta pausado:
        update_cpu_naive O(N^2)
    copiar Particle -> RenderParticle
    glBufferSubData al VBO
    dibujar GL_POINTS
    medir frame time y FPS
```

## Flujo de ejecucion OpenCL

```text
inicializar config
crear particulas
crear ventana OpenGL 1280x720
crear contexto OpenCL
crear buffers Particle A/B y RenderParticle
crear VBO OpenGL
mientras ventana abierta:
    procesar input
    si no esta pausado:
        ejecutar kernel update_particles_tiled O(N^2)
        cada work-group carga tiles en __local
        sincronizar con barrier(CLK_LOCAL_MEM_FENCE)
        intercambiar buffers A/B
    ejecutar o preparar buffer visual
    clEnqueueReadBuffer de RenderParticle
    glBufferSubData al VBO
    dibujar GL_POINTS
medir frame time y FPS
```

## Flujo de ejecucion CUDA

```text
inicializar config
crear particulas
crear ventana OpenGL 1280x720
crear buffers CUDA Particle A/B y RenderParticle
crear VBO OpenGL
mientras ventana abierta:
    procesar input
    si no esta pausado:
        ejecutar kernel update_particles_tiled_cuda O(N^2)
        cada bloque carga tiles en __shared__
        sincronizar con __syncthreads()
        intercambiar buffers A/B
    ejecutar kernel visual
    cudaMemcpy de RenderParticle hacia CPU
    glBufferSubData al VBO
    dibujar GL_POINTS
    medir frame time y FPS
```

## Comparacion esperada

Usar la misma simulacion y los mismos parametros para:

```text
N = 1024, 4096, 8192, 16384
```

Tabla final:

```text
N       CPU FPS   CPU ms   OpenCL FPS   OpenCL ms   CUDA FPS   CUDA ms
1024    ...       ...      ...          ...         ...        ...
4096    ...       ...      ...          ...         ...        ...
8192    ...       ...      ...          ...         ...        ...
16384   ...       ...      ...          ...         ...        ...
```
