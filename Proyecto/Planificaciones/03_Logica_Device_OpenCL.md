# Logica Device CUDA y OpenCL

## Alcance

Las versiones GPU mantienen complejidad `O(N^2)`, pero ya no deben leer todas las particulas directamente desde memoria global en cada comparacion.

El requisito del nucleo es:

- CUDA: usar memoria compartida `__shared__` y `__syncthreads()`.
- OpenCL: usar memoria local `__local` y `barrier(CLK_LOCAL_MEM_FENCE)`.

No se implementa:

- grilla espacial;
- prefix sum;
- interop OpenCL/OpenGL;
- estructuras de vecinos.

## Idea del algoritmo tiled

Cada hilo/work-item procesa una particula `i`.

En vez de recorrer `j = 0..N-1` leyendo siempre desde memoria global:

1. El bloque/work-group carga un tile de particulas a memoria rapida.
2. Se sincronizan los hilos.
3. Cada hilo compara su particula `i` contra todas las particulas del tile.
4. Se sincronizan nuevamente antes de cargar el siguiente tile.
5. Al final, cada hilo escribe solo `outParticles[i]`.

Esto sigue siendo `O(N^2)`, pero cumple el requisito de memoria compartida/local y reduce lecturas repetidas a memoria global.

## Structs compartidos

Mantener los mismos campos en C++, CUDA y OpenCL:

```c
#define MATERIAL_RED 0
#define MATERIAL_GREEN 1
#define MATERIAL_BLUE 2

#define STATE_NORMAL 0
#define STATE_ALTERED 1

typedef struct {
    float4 pos_radius;
    float4 vel_misc;
    int type;
    int state;
    float energy;
    int padding;
} Particle;

typedef struct {
    float x;
    float y;
    float radius;
    int type;
    float energy;
} RenderParticle;
```

## CUDA: kernel tiled con `__shared__`

Archivo:

```text
src/mainCUDA.cu
```

Estructura esperada:

```cpp
__global__ void update_particles_tiled_cuda(
    const Particle* inParticles,
    Particle* outParticles,
    float dt
) {
    __shared__ Particle tile[Config::LOCAL_SIZE];

    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int localId = threadIdx.x;
    bool active = i < Config::PARTICLE_COUNT;

    Particle p;
    if (active) {
        p = inParticles[i];
    }

    for (int tileStart = 0; tileStart < Config::PARTICLE_COUNT; tileStart += blockDim.x) {
        int j = tileStart + localId;

        if (j < Config::PARTICLE_COUNT) {
            tile[localId] = inParticles[j];
        }

        __syncthreads();

        int tileCount = min(blockDim.x, Config::PARTICLE_COUNT - tileStart);
        if (active) {
            for (int k = 0; k < tileCount; ++k) {
                int globalJ = tileStart + k;
                if (globalJ == i) continue;
                aplicar_interaccion_cuda(p, tile[k]);
            }
        }

        __syncthreads();
    }

    if (active) {
        integrar_y_rebotar_cuda(p, dt);
        outParticles[i] = p;
    }
}
```

Notas:

- `__shared__` es obligatorio.
- `__syncthreads()` debe ir despues de cargar el tile y antes de sobrescribirlo.
- Cada hilo escribe solo su propia particula.
- No hacer `return` antes de una barrera si el `globalSize` fue redondeado. Usar una bandera `active` para que todos los hilos del bloque lleguen a `__syncthreads()`.

## OpenCL: kernel tiled con `__local`

Archivo:

```text
kernels/kernel.cl
```

Estructura esperada:

```c
__kernel void update_particles_tiled(
    __global const Particle* inParticles,
    __global Particle* outParticles,
    const float dt,
    __local Particle* tile
) {
    int i = get_global_id(0);
    int localId = get_local_id(0);
    int localSize = get_local_size(0);
    int active = i < PARTICLE_COUNT;

    Particle p;
    if (active) {
        p = inParticles[i];
    }

    for (int tileStart = 0; tileStart < PARTICLE_COUNT; tileStart += localSize) {
        int j = tileStart + localId;

        if (j < PARTICLE_COUNT) {
            tile[localId] = inParticles[j];
        }

        barrier(CLK_LOCAL_MEM_FENCE);

        int tileCount = min(localSize, PARTICLE_COUNT - tileStart);
        if (active) {
            for (int k = 0; k < tileCount; ++k) {
                int globalJ = tileStart + k;
                if (globalJ == i) continue;
                aplicar_interaccion_opencl(&p, tile[k]);
            }
        }

        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (active) {
        integrar_y_rebotar_opencl(&p, dt);
        outParticles[i] = p;
    }
}
```

El host debe reservar memoria local dinamica:

```cpp
clSetKernelArg(kUpdate, 3, sizeof(Particle) * Config::LOCAL_SIZE, NULL);
```

Notas:

- `__local Particle* tile` es obligatorio.
- `barrier(CLK_LOCAL_MEM_FENCE)` es obligatorio despues de cargar el tile y antes de cargar el siguiente.
- La logica debe replicar la version CUDA.
- No hacer `return` antes de una barrera si el `globalSize` fue redondeado. Usar `active` para que todos los work-items del work-group lleguen a `barrier()`.

## Kernel visual

CUDA y OpenCL usan el mismo concepto:

```c
RenderParticle[i] = {
    particles[i].x,
    particles[i].y,
    particles[i].radius,
    particles[i].type,
    particles[i].energy
};
```

OpenCL:

```c
__kernel void build_render_particles(
    __global const Particle* particles,
    __global RenderParticle* renderParticles
);
```

CUDA:

```cpp
__global__ void build_render_particles_cuda(
    const Particle* particles,
    RenderParticle* renderParticles
);
```

## Tamanos de ejecucion

Usar el mismo tamaño de bloque/work-group:

```cpp
size_t localSize = Config::LOCAL_SIZE;
size_t globalSize =
    ((Config::PARTICLE_COUNT + localSize - 1) / localSize) * localSize;
```

En CUDA:

```cpp
int threads = Config::LOCAL_SIZE;
int blocks = (Config::PARTICLE_COUNT + threads - 1) / threads;
```

## Nota de alcance avanzado

Si el proyecto queda funcionando y sobra tiempo, se podria estudiar una grilla espacial en GPU. Pero no es parte del plan principal ni de la entrega base.
