# Planificacion de Reglas de Materiales CPU/CUDA

## Objetivo

Actualizar el comportamiento de las particulas para que los tres materiales se
sientan distintos:

- Verde: material liviano, acelera con facilidad.
- Azul: material normal, conserva su velocidad y recibe transferencia de la otra particula.
- Roja: material pesado, le cuesta ganar velocidad.

El cambio debe aplicarse en:

```text
Proyecto/SandboxParticulas/include/particles.hpp
Proyecto/SandboxParticulas/src/mainCUDA.cu
```

La version CPU y la version CUDA deben mantener reglas equivalentes para que la
comparacion de rendimiento siga siendo justa.

## Alcance

Se mantiene:

- simulacion `O(N^2)`;
- doble buffer;
- cada particula escribe solo su propio resultado;
- render con OpenGL;
- CPU secuencial;
- CUDA tiled con `__shared__`.

No se implementa:

- grilla espacial;
- interop CUDA/OpenGL;
- OpenCL como foco principal;
- fuerzas complejas tipo fisica realista;
- cambios de estructura de datos salvo que sean estrictamente necesarios.

## Regla Base de Colision

Se conserva la deteccion actual:

```cpp
const float dx = p.pos_radius.x - q.pos_radius.x;
const float dy = p.pos_radius.y - q.pos_radius.y;
const float dist2 = dx * dx + dy * dy;
const float minDist = p.pos_radius.z + q.pos_radius.z;

if (dist2 <= 0.000001f || dist2 >= minDist * minDist) {
    return;
}
```

Si hay colision, se calcula la normal:

```cpp
const float invDist = 1.0f / std::sqrt(dist2);
const float nx = dx * invDist;
const float ny = dy * invDist;
```

En CUDA se usa:

```cpp
const float invDist = rsqrtf(dist2);
```

## Nuevos Parametros Sugeridos

Agregar en `include/config.hpp` constantes para no dejar numeros magicos dentro
de las reglas:

```cpp
constexpr float MIN_PARTICLE_SPEED = 0.015f;
constexpr float MAX_PARTICLE_SPEED = 1.50f;

constexpr float GREEN_GREEN_SPEEDUP = 2.00f;
constexpr float GREEN_BLUE_SPEEDUP = 1.25f;
constexpr float GREEN_RED_SPEEDUP = 1.50f;

constexpr float BLUE_VELOCITY_TRANSFER = 1.00f;

constexpr float RED_GREEN_RESPONSE = 0.35f;
constexpr float RED_BLUE_RESPONSE = 0.50f;
constexpr float RED_RED_RESPONSE = 0.20f;
```

Interpretacion:

- `MIN_PARTICLE_SPEED`: velocidad minima para que ninguna particula quede quieta.
- `MAX_PARTICLE_SPEED`: limite superior para evitar explosiones numericas.
- `GREEN_*`: multiplicadores de velocidad para la particula verde segun con quien choque.
- `BLUE_VELOCITY_TRANSFER`: cuanto de la velocidad de la otra particula recibe la azul.
- `RED_*_RESPONSE`: cuanto reacciona la roja ante cada material; valores bajos porque es pesada.

Los valores pueden ajustarse despues de probar visualmente.

## Regla Verde: Material Liviano

La verde es la de menor peso. La idea es que gane velocidad al chocar.

Reglas propuestas:

```text
verde vs verde -> velocidad x2.00
verde vs azul  -> velocidad x1.25
verde vs roja  -> velocidad x1.50
```

Pseudocodigo CPU:

```cpp
if (p.type == MATERIAL_GREEN && q.type == MATERIAL_GREEN) {
    p.vel_misc.x *= Config::GREEN_GREEN_SPEEDUP;
    p.vel_misc.y *= Config::GREEN_GREEN_SPEEDUP;
    p.vel_misc.x += nx * 0.02f;
    p.vel_misc.y += ny * 0.02f;
    p.energy = 0.8f;
    p.state = STATE_ALTERED;
} else if (p.type == MATERIAL_GREEN && q.type == MATERIAL_BLUE) {
    p.vel_misc.x *= Config::GREEN_BLUE_SPEEDUP;
    p.vel_misc.y *= Config::GREEN_BLUE_SPEEDUP;
    p.vel_misc.x += nx * 0.02f;
    p.vel_misc.y += ny * 0.02f;
    p.energy = 0.7f;
    p.state = STATE_ALTERED;
} else if (p.type == MATERIAL_GREEN && q.type == MATERIAL_RED) {
    p.vel_misc.x *= Config::GREEN_RED_SPEEDUP;
    p.vel_misc.y *= Config::GREEN_RED_SPEEDUP;
    p.vel_misc.x += nx * 0.03f;
    p.vel_misc.y += ny * 0.03f;
    p.energy = 1.0f;
    p.state = STATE_ALTERED;
}
```

Notas:

- Verde contra roja gana mas velocidad que verde contra azul.
- Verde contra verde puede ser el caso mas caotico/liviano.
- El empuje por normal evita que solo se multiplique una direccion ya existente.

## Regla Azul: Material Normal

La azul conserva su velocidad y suma parte de la velocidad de la particula con
la que choca.

Regla propuesta:

```text
azul vs cualquiera -> vel_azul += vel_otro * BLUE_VELOCITY_TRANSFER
```

Para que no explote demasiado, tambien se suma un empuje moderado por normal.

Pseudocodigo CPU:

```cpp
else if (p.type == MATERIAL_BLUE) {
    p.vel_misc.x += q.vel_misc.x * Config::BLUE_VELOCITY_TRANSFER;
    p.vel_misc.y += q.vel_misc.y * Config::BLUE_VELOCITY_TRANSFER;
    p.vel_misc.x += nx * 0.02f;
    p.vel_misc.y += ny * 0.02f;
    p.energy = 0.6f;
}
```

Interpretacion:

- Si choca con una particula rapida, la azul hereda parte de ese movimiento.
- Si choca con una particula lenta, cambia poco.
- No es tan liviana como la verde, pero responde mas que la roja.

## Regla Roja: Material Pesado

La roja sera la mas pesada: no se queda inmovil, pero acelera poco.

Reglas propuestas:

```text
roja vs verde -> reaccion baja-media
roja vs azul  -> reaccion media
roja vs roja  -> reaccion muy baja
```

Pseudocodigo CPU:

```cpp
else if (p.type == MATERIAL_RED && q.type == MATERIAL_GREEN) {
    p.vel_misc.x += nx * Config::RED_GREEN_RESPONSE * 0.02f;
    p.vel_misc.y += ny * Config::RED_GREEN_RESPONSE * 0.02f;
    p.energy = 0.35f;
} else if (p.type == MATERIAL_RED && q.type == MATERIAL_BLUE) {
    p.vel_misc.x += nx * Config::RED_BLUE_RESPONSE * 0.02f;
    p.vel_misc.y += ny * Config::RED_BLUE_RESPONSE * 0.02f;
    p.energy = 0.45f;
} else if (p.type == MATERIAL_RED && q.type == MATERIAL_RED) {
    p.vel_misc.x += nx * Config::RED_RED_RESPONSE * 0.02f;
    p.vel_misc.y += ny * Config::RED_RED_RESPONSE * 0.02f;
    p.energy = 0.25f;
}
```

Interpretacion:

- Roja no recibe multiplicadores grandes.
- Roja se mueve y rebota, pero con menos aceleracion por choque.
- Visualmente deberia sentirse mas estable/pesada.

## Velocidad Minima

Requisito: ninguna particula puede llegar a velocidad `0`.

Se propone agregar una funcion auxiliar que se aplique despues de:

- interacciones;
- damping;
- rebotes;
- antes de guardar la particula final.

CPU:

```cpp
inline void limitar_velocidad_cpu(Particle& p) {
    const float vx = p.vel_misc.x;
    const float vy = p.vel_misc.y;
    const float speed2 = vx * vx + vy * vy;
    const float minSpeed = Config::MIN_PARTICLE_SPEED;
    const float maxSpeed = Config::MAX_PARTICLE_SPEED;

    if (speed2 < minSpeed * minSpeed) {
        if (speed2 <= 0.000001f) {
            p.vel_misc.x = minSpeed;
            p.vel_misc.y = 0.0f;
            return;
        }

        const float invSpeed = 1.0f / std::sqrt(speed2);
        p.vel_misc.x = vx * invSpeed * minSpeed;
        p.vel_misc.y = vy * invSpeed * minSpeed;
        return;
    }

    if (speed2 > maxSpeed * maxSpeed) {
        const float invSpeed = 1.0f / std::sqrt(speed2);
        p.vel_misc.x = vx * invSpeed * maxSpeed;
        p.vel_misc.y = vy * invSpeed * maxSpeed;
    }
}
```

CUDA:

```cpp
__device__ void limitar_velocidad_cuda(Particle& p) {
    const float vx = p.vel_misc.x;
    const float vy = p.vel_misc.y;
    const float speed2 = vx * vx + vy * vy;
    const float minSpeed = Config::MIN_PARTICLE_SPEED;
    const float maxSpeed = Config::MAX_PARTICLE_SPEED;

    if (speed2 < minSpeed * minSpeed) {
        if (speed2 <= 0.000001f) {
            p.vel_misc.x = minSpeed;
            p.vel_misc.y = 0.0f;
            return;
        }

        const float invSpeed = rsqrtf(speed2);
        p.vel_misc.x = vx * invSpeed * minSpeed;
        p.vel_misc.y = vy * invSpeed * minSpeed;
        return;
    }

    if (speed2 > maxSpeed * maxSpeed) {
        const float invSpeed = rsqrtf(speed2);
        p.vel_misc.x = vx * invSpeed * maxSpeed;
        p.vel_misc.y = vy * invSpeed * maxSpeed;
    }
}
```

Esta funcion no crea movimiento aleatorio; preserva la direccion cuando existe.
Si la velocidad es exactamente cero, empuja la particula hacia `+X` con velocidad
minima.

## Donde Llamar La Velocidad Minima

En CPU, dentro de `integrar_y_rebotar_cpu`, al final:

```cpp
p.energy = std::max(0.0f, p.energy - dt);
if (p.energy <= 0.0f) {
    p.state = STATE_NORMAL;
}

limitar_velocidad_cpu(p);
```

En CUDA, dentro de `integrar_y_rebotar_cuda`, al final:

```cpp
p.energy = fmaxf(0.0f, p.energy - dt);
if (p.energy <= 0.0f) {
    p.state = STATE_NORMAL;
}

limitar_velocidad_cuda(p);
```

## Orden de Implementacion

1. Agregar constantes nuevas en `include/config.hpp`.
2. Agregar `limitar_velocidad_cpu` en `include/particles.hpp`.
3. Reescribir `aplicar_interaccion_cpu` con las nuevas reglas.
4. Llamar `limitar_velocidad_cpu` al final de `integrar_y_rebotar_cpu`.
5. Agregar `limitar_velocidad_cuda` en `src/mainCUDA.cu`.
6. Reescribir `aplicar_interaccion_cuda` con las mismas reglas.
7. Llamar `limitar_velocidad_cuda` al final de `integrar_y_rebotar_cuda`.
8. Compilar CPU y CUDA.
9. Probar visualmente que ninguna particula quede quieta.

## Verificacion

Comandos:

```bash
make -B -C Proyecto/SandboxParticulas all3
make -C Proyecto/SandboxParticulas runCPU
make -C Proyecto/SandboxParticulas runCUDA
```

Criterios:

- CPU compila.
- CUDA compila.
- Las particulas verdes aceleran notoriamente.
- Las particulas azules heredan movimiento de otras particulas.
- Las particulas rojas se sienten mas pesadas/lentas.
- No quedan particulas detenidas en velocidad cero.
- El rendimiento sigue siendo comparable porque CPU y CUDA mantienen reglas equivalentes.

## Notas Para Ajuste Posterior

Si la simulacion se vuelve demasiado caotica:

- bajar `GREEN_GREEN_SPEEDUP`;
- bajar `MAX_PARTICLE_SPEED`;
- bajar `BLUE_VELOCITY_TRANSFER`.

Si la simulacion se siente muy quieta:

- subir `MIN_PARTICLE_SPEED`;
- subir `RED_*_RESPONSE`;
- subir `EMIT_SPEED`.

Si la roja se siente demasiado inmovil:

- subir `RED_GREEN_RESPONSE` y `RED_BLUE_RESPONSE`.

Si la verde se dispara demasiado:

- bajar `GREEN_GREEN_SPEEDUP`;
- bajar `GREEN_RED_SPEEDUP`;
- bajar `MAX_PARTICLE_SPEED`.
