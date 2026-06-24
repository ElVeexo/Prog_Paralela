# Checklist de Implementacion

## Fase 1: Estructura comun

- [ ] Crear `Proyecto/SandboxParticulas`.
- [ ] Crear `include/config.hpp`.
- [ ] Crear `include/particles.hpp`.
- [ ] Copiar `include/glad.h` desde `Lab 5/Pregunta2/src/glad.h`.
- [ ] Copiar `src/glad.c` desde `Lab 5/Pregunta2/src/glad.c`.
- [ ] Crear `src/gl_utils.hpp`.
- [ ] Crear `src/opencl_utils.hpp`.
- [ ] Crear `shaders/particles.vert`.
- [ ] Crear `shaders/particles.frag`.
- [ ] Crear `kernels/kernel.cl`.
- [ ] Crear `Makefile`.

## Fase 2: Render OpenGL comun

- [ ] Crear ventana `1280x720` usando `Config::WINDOW_WIDTH` y `Config::WINDOW_HEIGHT`.
- [ ] Crear shader de particulas.
- [ ] Crear VAO/VBO para `RenderParticle`.
- [ ] Renderizar con `GL_POINTS`.
- [ ] Confirmar que se ven particulas circulares y coloreadas.

## Fase 3: Version CPU con ventana

- [ ] Crear `src/mainCPU.cpp`.
- [ ] Inicializar particulas con `crear_particulas`.
- [ ] Implementar `update_cpu_naive O(N^2)`.
- [ ] Usar doble buffer CPU: `particlesIn` y `particlesOut`.
- [ ] Convertir `Particle` a `RenderParticle`.
- [ ] Actualizar VBO con `glBufferSubData`.
- [ ] Medir FPS y frame time con `<chrono>`.
- [ ] Implementar `SPACE`, `R`, `1`, `2`, `3`.

## Fase 4: Version GPU con ventana

- [ ] Crear `src/main.cpp`.
- [ ] Inicializar OpenCL siguiendo `Lab 5/Pregunta1/main.cpp`.
- [ ] Compilar `kernels/kernel.cl`.
- [ ] Crear `d_particlesIn`, `d_particlesOut`, `d_renderParticles`.
- [ ] Implementar kernel `update_particles_tiled`.
- [ ] Usar memoria local `__local Particle* tile`.
- [ ] Reservar memoria local desde host con `clSetKernelArg`.
- [ ] Usar `barrier(CLK_LOCAL_MEM_FENCE)` despues de cargar cada tile.
- [ ] Implementar kernel `build_render_particles`.
- [ ] Leer `d_renderParticles` con `clEnqueueReadBuffer`.
- [ ] Actualizar VBO con `glBufferSubData`.
- [ ] Medir FPS y frame time con `<chrono>`.
- [ ] Implementar `SPACE`, `R`, `1`, `2`, `3`.

## Fase 5: Version CUDA con ventana

- [ ] Crear `src/mainCUDA.cu`.
- [ ] Implementar kernel CUDA `update_particles_tiled_cuda`.
- [ ] Usar memoria compartida `__shared__ Particle tile[...]`.
- [ ] Usar `__syncthreads()` despues de cargar cada tile.
- [ ] Implementar kernel CUDA `build_render_particles_cuda`.
- [ ] Leer `dRenderParticles` con `cudaMemcpy`.
- [ ] Actualizar VBO con `glBufferSubData`.
- [ ] Medir FPS y frame time con `<chrono>`.

## Fase 6: Comparacion

- [ ] Probar `N = 1024`.
- [ ] Probar `N = 4096`.
- [ ] Probar `N = 8192`.
- [ ] Probar `N = 16384`.
- [ ] Registrar CPU FPS y CPU frame ms.
- [ ] Registrar OpenCL FPS y OpenCL frame ms.
- [ ] Registrar CUDA FPS y CUDA frame ms.
- [ ] Crear tabla comparativa.

## Fase 7: Presentacion

- [ ] Mostrar demo CPU.
- [ ] Mostrar demo OpenCL.
- [ ] Mostrar demo CUDA.
- [ ] Explicar que las tres versiones usan OpenGL y mismas reglas.
- [ ] Explicar que CPU es secuencial `O(N^2)`.
- [ ] Explicar que OpenCL usa kernel tiled `O(N^2)` con `__local`.
- [ ] Explicar que CUDA usa kernel tiled `O(N^2)` con `__shared__`.
- [ ] Explicar que grilla espacial e interop quedan fuera de scope.
