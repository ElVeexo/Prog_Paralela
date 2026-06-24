# Sandbox de Particulas

Tres versiones comparables:

- `sandboxCPU`: simulacion secuencial `O(N^2)` + OpenGL.
- `sandboxGPU`: simulacion OpenCL tiled `O(N^2)` + `__local` + OpenGL.
- `sandboxCUDA`: simulacion CUDA tiled `O(N^2)` + `__shared__` + OpenGL.

Ambas usan:

- `include/config.hpp` para parametros globales.
- `include/particles.hpp` para estructuras y reglas compartidas.
- `GL_POINTS` para renderizar particulas en tiempo real.

## Compilar

```bash
make
```

Por defecto `make` compila CPU + OpenCL. CUDA se deja separado para que el proyecto no falle en computadores sin `nvcc`.

Para compilar las tres versiones:

```bash
make all3
```

Para compilar solo CUDA:

```bash
make cuda
```

## Dependencias

El proyecto ahora incluye dos backends GPU:

- OpenCL: `src/main.cpp` + `kernels/kernel.cl`.
- CUDA: `src/mainCUDA.cu`.

Paquetes base en Ubuntu/Debian:

```bash
make install-deps-ubuntu
```

Eso instala compilador C/C++, GLFW, OpenGL/Mesa, headers de OpenCL, loader OpenCL y `clinfo`. Es suficiente para `sandboxCPU` y para compilar `sandboxGPU`.

Para compilar y ejecutar `sandboxCUDA`, instalar CUDA toolkit:

```bash
make install-cuda-ubuntu
```

Luego se puede revisar si OpenCL ve algun dispositivo:

```bash
make check-opencl
```

Para revisar CUDA:

```bash
make check-cuda
```

Notas:

- `sandboxCPU` necesita GLFW/OpenGL, pero no necesita OpenCL ni CUDA.
- `sandboxGPU` necesita GLFW/OpenGL y un runtime OpenCL funcional.
- `sandboxCUDA` necesita GLFW/OpenGL, `nvcc`, CUDA runtime y una GPU NVIDIA compatible.
- En equipos sin GPU compatible, OpenCL puede usar CPU como fallback si hay runtime CPU instalado; CUDA no tiene fallback CPU.

## Ejecutar

```bash
make runCPU
make runGPU
make runCUDA
```

## Controles

- `SPACE`: pausar/reanudar.
- `R`: reiniciar.
- `1`: emitir particulas rojas.
- `2`: emitir particulas verdes.
- `3`: emitir particulas azules.
- Click izquierdo: emitir particulas alrededor del cursor.
- `ESC`: cerrar.

## Comparacion

Cambiar `Config::PARTICLE_COUNT` en `include/config.hpp`, recompilar y registrar:

```text
N = 1024, 4096, 8192, 16384
```

Tabla sugerida:

```text
N       CPU FPS   CPU ms   OpenCL FPS   OpenCL ms   CUDA FPS   CUDA ms
1024    ...       ...      ...          ...         ...        ...
4096    ...       ...      ...          ...         ...        ...
8192    ...       ...      ...          ...         ...        ...
16384   ...       ...      ...          ...         ...        ...
```
