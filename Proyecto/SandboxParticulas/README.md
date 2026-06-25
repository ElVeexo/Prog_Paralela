# Sandbox de Particulas

Sandbox interactivo 2D con particulas de distintos materiales. El proyecto se
trabajara principalmente con dos versiones comparables:

- `sandboxCPU`: simulacion secuencial `O(N^2)` + OpenGL.
- `sandboxCUDA`: simulacion CUDA tiled `O(N^2)` + `__shared__` + OpenGL.

La version OpenCL puede quedar en el repositorio como referencia/experimento,
pero no sera el foco principal debido a las limitaciones de OpenCL con GPU NVIDIA
en WSL. Para la comparacion del proyecto se usara CPU secuencial vs CUDA.

Ambas versiones principales usan:

- `include/config.hpp` para parametros globales.
- `include/particles.hpp` para estructuras, inicializacion y reglas CPU.
- `src/mainCUDA.cu` para reglas CUDA equivalentes.
- `GL_POINTS` para renderizar particulas en tiempo real.

## Compilar

Para compilar CPU:

```bash
make
```

Nota: el target `make` tambien puede compilar OpenCL si el `Makefile` mantiene
ese backend. Para el proyecto principal, los ejecutables importantes son
`sandboxCPU` y `sandboxCUDA`.

Para compilar CUDA:

```bash
make cuda
```

Para compilar todo lo disponible:

```bash
make all3
```

## Dependencias

En Ubuntu/Debian/WSL:

```bash
make install-all
```

Para revisar CUDA:

```bash
make check-cuda
```

Notas:

- `sandboxCPU` necesita GLFW/OpenGL, pero no necesita CUDA.
- `sandboxCUDA` necesita GLFW/OpenGL, `nvcc`, CUDA runtime y una GPU NVIDIA compatible.
- En WSL, CUDA funciona para GPU NVIDIA; OpenCL puede no exponer la GPU NVIDIA y puede caer a CPU.

## Ejecutar

```bash
make runCPU
make runCUDA
```

Para ejecutar con una cantidad especifica de particulas sin editar
`include/config.hpp`, modificar esta variable en el `Makefile`:

```makefile
particle_N := 4096
```

Luego ejecutar:

```bash
make runCPU_N
make runCUDA_N
```

Tambien existen targets rapidos:

```bash
make runCPU_4096
make runCUDA_4096
```

Opcionalmente, se puede sobrescribir desde el comando:

```bash
make runCPU_N particle_N=4096
make runCUDA_N particle_N=4096
```

## Controles

- `SPACE`: pausar/reanudar.
- `R`: reiniciar.
- `1`: seleccionar particulas rojas.
- `2`: seleccionar particulas verdes.
- `3`: seleccionar particulas azules.
- Click izquierdo: emitir una particula del color seleccionado en el cursor.
- `ESC`: cerrar.

Cada cambio de color se imprime en consola como:

```text
[INFO] Color seleccionado: rojo
```

## Configuracion

Todas las variables generales se modifican en:

```text
include/config.hpp
```

Variables principales:

- `WINDOW_WIDTH`: ancho inicial de la ventana OpenGL.
- `WINDOW_HEIGHT`: alto inicial de la ventana OpenGL.
- `PARTICLE_COUNT`: cantidad total de particulas simuladas. Aumentar este valor sube el costo `O(N^2)`.
- `LOCAL_SIZE`: hilos por bloque CUDA y tamano del tile compartido.
- `WORLD_MIN_X`, `WORLD_MAX_X`: limites horizontales del mundo.
- `WORLD_MIN_Y`, `WORLD_MAX_Y`: limites verticales del mundo.
- `BASE_RADIUS`: radio inicial de cada particula.
- `DEFAULT_DAMPING`: amortiguacion aplicada a la velocidad en cada frame.
- `WALL_BOUNCE`: perdida/conservacion de velocidad al rebotar con paredes.
- `FIXED_DT`: paso de tiempo fijo de la simulacion.
- `MIN_PARTICLE_SPEED`: velocidad minima para que ninguna particula quede detenida.
- `MAX_PARTICLE_SPEED`: velocidad maxima para evitar explosiones numericas.
- `GREEN_GREEN_SPEEDUP`: aceleracion de verde al chocar con verde.
- `GREEN_BLUE_SPEEDUP`: aceleracion de verde al chocar con azul.
- `GREEN_RED_SPEEDUP`: aceleracion de verde al chocar con rojo.
- `BLUE_VELOCITY_TRANSFER`: cuanto de la velocidad de la otra particula hereda la azul.
- `BLUE_COLLISION_DAMPING`: amortiguacion extra que recibe la azul al chocar.
- `BLUE_NORMAL_PUSH`: empuje propio de la azul al separarse de otra particula.
- `RED_GREEN_RESPONSE`: respuesta de la roja al chocar con verde.
- `RED_BLUE_RESPONSE`: respuesta de la roja al chocar con azul.
- `RED_RED_RESPONSE`: respuesta de la roja al chocar con roja.
- `EMIT_COUNT`: cantidad de particulas emitidas por click. Para este proyecto queda en `1`.
- `EMIT_SPREAD`: dispersion aleatoria alrededor del cursor al emitir.
- `EMIT_SPEED`: rango de velocidad inicial de la particula emitida.
- `WARMUP_FRAMES`: frames ignorados antes de promediar mediciones.
- `MEASURE_FRAMES`: cantidad objetivo de frames de medicion.
- `LOG_EVERY_FRAMES`: frecuencia con que se imprimen FPS y frame time.

Para la tabla comparativa, cambiar `Config::PARTICLE_COUNT`, recompilar y medir:

```text
N = 1024, 4096, 8192, 16384
```

## Reglas de Particulas

La explicacion detallada de campos, estados, colisiones y ejemplos de cambios
esta en:

```text
GUIA_REGLAS_PARTICULAS.md
```

Las estructuras compartidas estan en:

```text
include/particles.hpp
```

Ahi se definen:

- `MaterialType`: rojo, verde y azul.
- `Particle`: datos de simulacion.
- `RenderParticle`: datos enviados a OpenGL.
- `crear_particulas`: inicializacion comun.
- `make_particle`: creacion de una particula.
- `aplicar_interaccion_cpu`: reglas de colision/interaccion en CPU.
- `integrar_y_rebotar_cpu`: movimiento, damping y rebote en paredes.

Para cambiar el comportamiento en CPU, editar:

```text
include/particles.hpp
```

Funciones clave:

- `aplicar_interaccion_cpu`
- `integrar_y_rebotar_cpu`

Para cambiar el comportamiento en CUDA, editar:

```text
src/mainCUDA.cu
```

Funciones clave:

- `aplicar_interaccion_cuda`
- `integrar_y_rebotar_cuda`
- `update_particles_tiled_cuda`

Importante: si se cambia una regla en CPU, se debe replicar el mismo cambio en
CUDA para que la comparacion sea justa.

## Comparacion

Tabla sugerida:

```text
N       CPU FPS   CPU ms   CUDA FPS   CUDA ms
1024    ...       ...      ...        ...
4096    ...       ...      ...        ...
8192    ...       ...      ...        ...
16384   ...       ...      ...        ...
```

El objetivo es comparar la version secuencial `O(N^2)` contra la version CUDA
tiled `O(N^2)` usando memoria compartida.
