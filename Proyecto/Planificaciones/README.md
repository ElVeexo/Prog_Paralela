# Planificaciones del Sandbox de Particulas

Esta planificacion queda acotada al alcance real del proyecto:

```text
CPU O(N^2) + OpenGL
GPU CUDA tiled O(N^2) + OpenGL + __shared__
GPU OpenCL tiled O(N^2) + OpenGL + __local/barrier
misma configuracion, mismos datos, mismas reglas, misma visualizacion
```

Orden recomendado de lectura:

1. `00_Resumen_Arquitectura.md`: alcance final y estructura minima.
2. `01_Estructura_Datos.md`: `config.hpp`, `Particle`, `RenderParticle` y reglas.
3. `02_Logica_Host_CPP.md`: host OpenCL con kernel tiled y memoria local.
4. `03_Logica_Device_OpenCL.md`: kernels CUDA/OpenCL tiled con `__shared__`/`__local`.
5. `04_Render_OpenGL.md`: render comun para CPU y GPU con `GL_POINTS`.
6. `05_CPU_Secuencial_y_Mediciones.md`: version CPU con ventana y tabla comparativa.
7. `06_Checklist_Implementacion.md`: checklist realista por fases.

Fuera del nucleo:

- Grilla espacial.
- Interoperabilidad OpenCL/OpenGL con VBO compartido.
- Prefix sum paralelo.

Esas ideas pueden mencionarse como trabajo futuro, pero no se planifican como parte de la implementacion principal.
