# Estructura de Datos y Configuracion

## Regla principal

CPU y GPU deben compartir:

- mismo `config.hpp`;
- mismo `Particle`;
- mismo `RenderParticle`;
- misma inicializacion;
- mismas reglas de interaccion.

Asi la comparacion de FPS y frame time es justa.

## `include/config.hpp`

Archivo unico de configuracion. Si se cambia `N`, resolucion, radio o cantidad de frames medidos, se cambia aqui y afecta a las tres versiones.

```cpp
#pragma once

namespace Config {
    constexpr int WINDOW_WIDTH = 1280;
    constexpr int WINDOW_HEIGHT = 720;

    constexpr int PARTICLE_COUNT = 8192;

    constexpr float WORLD_MIN_X = -1.0f;
    constexpr float WORLD_MAX_X = 1.0f;
    constexpr float WORLD_MIN_Y = -1.0f;
    constexpr float WORLD_MAX_Y = 1.0f;

    constexpr float BASE_RADIUS = 0.006f;
    constexpr float DEFAULT_DAMPING = 0.995f;
    constexpr float WALL_BOUNCE = 0.90f;

    constexpr int LOCAL_SIZE = 256;
    constexpr int TILE_SIZE = LOCAL_SIZE;
    constexpr int WARMUP_FRAMES = 30;
    constexpr int MEASURE_FRAMES = 300;
}
```

Para probar distintos tamanos:

```cpp
constexpr int PARTICLE_COUNT = 1024;
constexpr int PARTICLE_COUNT = 4096;
constexpr int PARTICLE_COUNT = 8192;
constexpr int PARTICLE_COUNT = 16384;
```

Se cambia una vez, se recompilan ambos ejecutables y se registra la tabla.

## Materiales y estados

```cpp
enum MaterialType {
    MATERIAL_RED = 0,
    MATERIAL_GREEN = 1,
    MATERIAL_BLUE = 2
};

enum ParticleState {
    STATE_NORMAL = 0,
    STATE_ALTERED = 1
};
```

En OpenCL usar los mismos valores con `#define`:

```c
#define MATERIAL_RED 0
#define MATERIAL_GREEN 1
#define MATERIAL_BLUE 2

#define STATE_NORMAL 0
#define STATE_ALTERED 1
```

## `Particle`

Usar `float4`/`cl_float4` para mantener alineacion razonable entre C++ y OpenCL.

```cpp
#include <CL/cl.h>

struct Particle {
    cl_float4 pos_radius;  // x, y, radius, unused
    cl_float4 vel_misc;    // vx, vy, damping, unused
    cl_int type;
    cl_int state;
    cl_float energy;
    cl_int padding;
};
```

Campos:

- `pos_radius.x`: posicion X.
- `pos_radius.y`: posicion Y.
- `pos_radius.z`: radio.
- `vel_misc.x`: velocidad X.
- `vel_misc.y`: velocidad Y.
- `vel_misc.z`: damping.
- `type`: rojo, verde o azul.
- `state`: normal o alterado.
- `energy`: intensidad visual temporal.

## `RenderParticle`

Buffer compacto para OpenGL:

```cpp
struct RenderParticle {
    float x;
    float y;
    float radius;
    int type;
    float energy;
};
```

CPU:

- Convierte `Particle` a `RenderParticle` directamente en C++.

OpenCL:

- OpenCL genera `RenderParticle`.
- Host lee con `clEnqueueReadBuffer`.
- Host actualiza el VBO con `glBufferSubData`.

CUDA:

- CUDA genera `RenderParticle`.
- Host lee con `cudaMemcpy`.
- Host actualiza el VBO con `glBufferSubData`.

## Inicializacion comun

Crear funcion compartida:

```cpp
std::vector<Particle> crear_particulas(int count, unsigned int seed);
```

Reglas:

- posiciones aleatorias dentro de `[-0.95, 0.95]`;
- velocidades pequenas;
- tipos repartidos entre rojo, verde y azul;
- radio `Config::BASE_RADIUS`;
- damping `Config::DEFAULT_DAMPING`;
- energia inicial `0.0f`;
- estado inicial `STATE_NORMAL`.

## Reglas de interaccion

Mantener pocas reglas para que sean faciles de implementar igual en CPU y OpenCL:

- Mismo material: rebote suave.
- Roja contra verde: la particula rota su velocidad y gana energia.
- Verde contra azul: pierde velocidad.
- Azul contra roja: cambia a estado alterado y aumenta energia.

Condicion de colision:

```text
distancia^2 < (radio_i + radio_j)^2
```

Regla anti race condition en GPU:

- cada work-item calcula solo la nueva version de su propia particula `i`;
- lee todas las particulas desde el buffer anterior, cargadas por tiles a `__shared__` en CUDA o `__local` en OpenCL;
- escribe solo en `outParticles[i]`.
