# Guia de Reglas de Particulas

Esta guia explica como funcionan las particulas del sandbox y donde modificar su
comportamiento. La idea importante es esta:

- CPU usa las reglas en `include/particles.hpp`.
- CUDA usa una copia equivalente de esas reglas en `src/mainCUDA.cu`.
- Si cambias una regla en CPU, debes replicarla en CUDA para que la comparacion sea justa.

## Por Que `particles.hpp` y No `particles.cpp`

`particles.hpp` es un header porque contiene estructuras y funciones pequeñas que
se comparten entre varios archivos:

- `src/mainCPU.cpp`
- `src/mainCUDA.cu`
- otros helpers de render/input

Varias funciones estan marcadas como `inline`:

```cpp
inline void aplicar_interaccion_cpu(Particle& p, const Particle& q) {
    // ...
}
```

Eso permite definirlas directamente en el `.hpp` sin provocar errores de
"multiple definition" cuando el header se incluye desde mas de un archivo.

En un proyecto mas grande se podria separar declaracion en `.hpp` e
implementacion en `.cpp`, pero aqui mantenerlo en el header simplifica compartir
structs y logica comun.

## Estructura `Particle`

La estructura principal esta en `include/particles.hpp`:

```cpp
struct Particle {
    Float4 pos_radius;
    Float4 vel_misc;
    std::int32_t type;
    std::int32_t state;
    float energy;
    std::int32_t padding;
};
```

Sus campos se usan asi:

```text
pos_radius.x = posicion X
pos_radius.y = posicion Y
pos_radius.z = radio visual/fisico
pos_radius.w = sin uso

vel_misc.x = velocidad X
vel_misc.y = velocidad Y
vel_misc.z = damping/amortiguacion
vel_misc.w = sin uso

type   = material/color de la particula
state  = estado logico/visual
energy = intensidad temporal para brillo/cambio visual
padding = relleno para mantener layout/alineacion
```

Los materiales actuales son:

```cpp
enum MaterialType {
    MATERIAL_RED = 0,
    MATERIAL_GREEN = 1,
    MATERIAL_BLUE = 2
};
```

Los estados actuales son:

```cpp
enum ParticleState {
    STATE_NORMAL = 0,
    STATE_ALTERED = 1
};
```

`STATE_ALTERED` hoy no cambia la fisica por si solo. Se usa como marca de que
una particula fue alterada recientemente. En el codigo actual, cuando `energy`
llega a `0`, el estado vuelve a `STATE_NORMAL`:

```cpp
p.energy = std::max(0.0f, p.energy - dt);
if (p.energy <= 0.0f) {
    p.state = STATE_NORMAL;
}
```

El brillo visible depende de `energy`, no directamente de `state`. Al final de
cada integracion tambien se limita la velocidad para que ninguna particula quede
detenida ni se dispare sin control:

```cpp
limitar_velocidad_cpu(p);
```

## Flujo de Simulacion

En CPU, cada frame llama:

```cpp
update_cpu_naive(particlesIn, particlesOut, Config::FIXED_DT);
```

La funcion hace una comparacion `O(N^2)`:

```cpp
for (int i = 0; i < count; ++i) {
    Particle p = in[i];

    for (int j = 0; j < count; ++j) {
        if (i == j) {
            continue;
        }
        aplicar_interaccion_cpu(p, in[j]);
    }

    integrar_y_rebotar_cpu(p, dt);
    out[i] = p;
}
```

Traduccion:

1. Se toma una particula `p`.
2. Se compara contra todas las demas particulas `q`.
3. Si colisiona con alguna, se modifica solo `p`.
4. Al final se integra movimiento/rebote.
5. Se escribe el resultado en `out[i]`.

Esto evita que dos particulas escriban el mismo dato a la vez. En CUDA se usa la
misma idea: cada hilo escribe solo su propia particula.

## Como Se Detecta Una Colision

En `aplicar_interaccion_cpu`:

```cpp
const float dx = p.pos_radius.x - q.pos_radius.x;
const float dy = p.pos_radius.y - q.pos_radius.y;
const float dist2 = dx * dx + dy * dy;
const float minDist = p.pos_radius.z + q.pos_radius.z;

if (dist2 <= 0.000001f || dist2 >= minDist * minDist) {
    return;
}
```

Hay colision/interaccion si:

```text
distancia^2 < (radio_p + radio_q)^2
```

Se usa `dist2` para evitar calcular raiz cuadrada antes de saber si hay choque.

Luego se calcula una normal de separacion:

```cpp
const float invDist = 1.0f / std::sqrt(dist2);
const float nx = dx * invDist;
const float ny = dy * invDist;
```

`nx` y `ny` indican hacia donde empujar `p` para separarla de `q`.

## Reglas Actuales

Las reglas actuales intentan que cada material tenga una personalidad:

- Verde: particula liviana, gana velocidad rapidamente.
- Azul: particula normal, conserva su velocidad y suma movimiento de la otra particula.
- Roja: particula pesada, responde poco a los choques.

Los numeros se ajustan desde `include/config.hpp`:

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

### Verde: Material Liviano

La verde gana velocidad al chocar:

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

Efecto:

- verde con verde acelera fuerte (`x2.00`);
- verde con azul acelera moderado (`x1.25`);
- verde con rojo acelera alto (`x1.50`);
- `energy` sube para verse mas brillante.

### Azul: Material Normal

La azul conserva su velocidad y suma la velocidad de la particula con la que
choca:

```cpp
else if (p.type == MATERIAL_BLUE) {
    p.vel_misc.x += q.vel_misc.x * Config::BLUE_VELOCITY_TRANSFER;
    p.vel_misc.y += q.vel_misc.y * Config::BLUE_VELOCITY_TRANSFER;
    p.vel_misc.x += nx * 0.02f;
    p.vel_misc.y += ny * 0.02f;
    p.energy = 0.6f;
    p.state = STATE_ALTERED;
}
```

Efecto:

- si choca con algo rapido, hereda bastante movimiento;
- si choca con algo lento, cambia poco;
- funciona como material intermedio.

### Roja: Material Pesado

La roja no multiplica velocidad. Solo recibe empujes pequeños segun el material:

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

Efecto:

- roja contra verde reacciona poco;
- roja contra azul reacciona un poco mas;
- roja contra roja casi no acelera;
- visualmente deberia sentirse mas pesada.

## Movimiento y Rebote

Despues de revisar colisiones, se llama:

```cpp
integrar_y_rebotar_cpu(p, dt);
```

Primero actualiza posicion:

```cpp
p.pos_radius.x += p.vel_misc.x * dt;
p.pos_radius.y += p.vel_misc.y * dt;
```

Luego aplica amortiguacion:

```cpp
p.vel_misc.x *= p.vel_misc.z;
p.vel_misc.y *= p.vel_misc.z;
```

`p.vel_misc.z` viene de:

```cpp
Config::DEFAULT_DAMPING
```

Si la particula toca los limites del mundo, se reposiciona y rebota:

```cpp
if (p.pos_radius.x < Config::WORLD_MIN_X + radius) {
    p.pos_radius.x = Config::WORLD_MIN_X + radius;
    p.vel_misc.x *= -Config::WALL_BOUNCE;
}
```

`Config::WALL_BOUNCE` controla cuanta velocidad conserva al rebotar.

Al final se llama:

```cpp
limitar_velocidad_cpu(p);
```

Esto mantiene la velocidad dentro de:

```text
Config::MIN_PARTICLE_SPEED <= velocidad <= Config::MAX_PARTICLE_SPEED
```

Si una particula llega a velocidad cero, se le asigna una velocidad minima hacia
`+X` para que vuelva a moverse.

## Como Cambiar Una Regla

La forma mas simple es editar `aplicar_interaccion_cpu` en:

```text
include/particles.hpp
```

Ejemplo: hacer que roja contra verde duplique la velocidad de la roja:

```cpp
if (p.type == MATERIAL_RED && q.type == MATERIAL_GREEN) {
    p.vel_misc.x *= 2.0f;
    p.vel_misc.y *= 2.0f;
    p.energy = 1.0f;
    p.state = STATE_ALTERED;
}
```

Para que verde contra azul se ralentice mucho:

```cpp
else if (p.type == MATERIAL_GREEN && q.type == MATERIAL_BLUE) {
    p.vel_misc.x *= 0.50f;
    p.vel_misc.y *= 0.50f;
    p.energy = 0.5f;
}
```

Para que azul contra roja rebote mas fuerte:

```cpp
else if (p.type == MATERIAL_BLUE && q.type == MATERIAL_RED) {
    p.vel_misc.x += nx * 0.10f;
    p.vel_misc.y += ny * 0.10f;
    p.energy = 1.0f;
    p.state = STATE_ALTERED;
}
```

## Ejemplo: Una Acelera y La Otra Se Ralentiza

En este proyecto, cada llamada a `aplicar_interaccion_cpu(p, q)` modifica solo
`p`, no modifica `q`.

Eso significa que si roja y verde chocan:

- cuando `p` sea la roja y `q` la verde, se aplica la regla roja contra verde;
- en otro momento del doble loop, cuando `p` sea la verde y `q` la roja, puedes
  aplicar la regla verde contra roja.

Ejemplo completo:

```cpp
if (p.type == MATERIAL_RED && q.type == MATERIAL_GREEN) {
    p.vel_misc.x *= 2.0f;
    p.vel_misc.y *= 2.0f;
    p.energy = 1.0f;
    p.state = STATE_ALTERED;
} else if (p.type == MATERIAL_GREEN && q.type == MATERIAL_RED) {
    p.vel_misc.x *= 0.50f;
    p.vel_misc.y *= 0.50f;
    p.energy = 0.4f;
}
```

Resultado:

- la roja se acelera al chocar con verde;
- la verde se ralentiza al chocar con roja.

Esto no es aleatorio; es determinista por color.

## Aleatoriedad: Que Una De Las Dos Cambie Al Azar

En CPU podrias usar aleatoriedad real, pero en CUDA no conviene usar `std::random`
dentro del kernel. Para mantener CPU y CUDA parecidos, es mejor usar una funcion
pseudoaleatoria determinista basada en los datos de las particulas.

Ejemplo CPU simple:

```cpp
inline bool pseudo_random_choice(const Particle& p, const Particle& q) {
    const int a = static_cast<int>((p.pos_radius.x + 1.0f) * 10000.0f);
    const int b = static_cast<int>((q.pos_radius.y + 1.0f) * 10000.0f);
    return ((a * 73856093) ^ (b * 19349663)) & 1;
}
```

Luego:

```cpp
if (p.type == MATERIAL_RED && q.type == MATERIAL_GREEN) {
    if (pseudo_random_choice(p, q)) {
        p.vel_misc.x *= 2.0f;
        p.vel_misc.y *= 2.0f;
        p.energy = 1.0f;
    } else {
        p.vel_misc.x *= 0.50f;
        p.vel_misc.y *= 0.50f;
        p.energy = 0.4f;
    }
    p.state = STATE_ALTERED;
}
```

Para CUDA, se debe crear la misma funcion como `__device__` en `src/mainCUDA.cu`:

```cpp
__device__ bool pseudo_random_choice_cuda(const Particle& p, const Particle& q) {
    const int a = static_cast<int>((p.pos_radius.x + 1.0f) * 10000.0f);
    const int b = static_cast<int>((q.pos_radius.y + 1.0f) * 10000.0f);
    return ((a * 73856093) ^ (b * 19349663)) & 1;
}
```

Y usarla dentro de `aplicar_interaccion_cuda`.

## Como Replicar La Regla En CUDA

Si cambias esto en CPU:

```cpp
if (p.type == MATERIAL_RED && q.type == MATERIAL_GREEN) {
    p.vel_misc.x *= 2.0f;
    p.vel_misc.y *= 2.0f;
    p.energy = 1.0f;
    p.state = STATE_ALTERED;
}
```

Debes cambiar lo equivalente en `src/mainCUDA.cu`:

```cpp
__device__ void aplicar_interaccion_cuda(Particle& p, const Particle& q) {
    // deteccion de colision...

    if (p.type == MATERIAL_RED && q.type == MATERIAL_GREEN) {
        p.vel_misc.x *= 2.0f;
        p.vel_misc.y *= 2.0f;
        p.energy = 1.0f;
        p.state = STATE_ALTERED;
    }
}
```

La regla debe ser igual para que CPU y CUDA hagan la misma simulacion.

## Que Cambiar Para Nuevos Comportamientos

Cambios comunes:

- Mas velocidad: multiplicar `p.vel_misc.x/y` por un valor mayor que `1.0f`.
- Menos velocidad: multiplicar `p.vel_misc.x/y` por un valor entre `0.0f` y `1.0f`.
- Empuje direccional: sumar `nx * fuerza` y `ny * fuerza`.
- Brillo temporal: subir `p.energy`.
- Estado alterado: asignar `p.state = STATE_ALTERED`.
- Cambiar material: asignar `p.type = MATERIAL_RED`, `MATERIAL_GREEN` o `MATERIAL_BLUE`.

Ejemplo: una particula azul se convierte en roja al chocar con verde:

```cpp
else if (p.type == MATERIAL_BLUE && q.type == MATERIAL_GREEN) {
    p.type = MATERIAL_RED;
    p.energy = 1.0f;
    p.state = STATE_ALTERED;
}
```

Ejemplo: verdes se repelen muy fuerte entre ellas:

```cpp
else if (p.type == MATERIAL_GREEN && q.type == MATERIAL_GREEN) {
    p.vel_misc.x += nx * 0.12f;
    p.vel_misc.y += ny * 0.12f;
    p.energy = 0.7f;
}
```

## Checklist Para Editar Reglas

1. Cambiar `aplicar_interaccion_cpu` en `include/particles.hpp`.
2. Cambiar `aplicar_interaccion_cuda` en `src/mainCUDA.cu`.
3. Recompilar:

```bash
make -B -C Proyecto/SandboxParticulas all3
```

4. Probar:

```bash
make -C Proyecto/SandboxParticulas runCPU
make -C Proyecto/SandboxParticulas runCUDA
```

5. Comparar que ambas versiones se comporten parecido.
