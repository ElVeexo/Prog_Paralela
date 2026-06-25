#pragma once

namespace Config {
constexpr int WINDOW_WIDTH = 600;
constexpr int WINDOW_HEIGHT = 600;

#ifdef PARTICLE_COUNT_OVERRIDE
constexpr int PARTICLE_COUNT = PARTICLE_COUNT_OVERRIDE; // Permite colocar la cantidad de partículas desde la línea de comandos o desde el makefile
#else
constexpr int PARTICLE_COUNT = 8192;
#endif
constexpr int LOCAL_SIZE = 256; // Cantidad de hebras por bloque

// Limites del mundo en coordenadas normalizadas (van del -1.0f al 1.0f)
constexpr float WORLD_MIN_X = -1.0f;
constexpr float WORLD_MAX_X = 1.0f;
constexpr float WORLD_MIN_Y = -1.0f;
constexpr float WORLD_MAX_Y = 1.0f;

constexpr float BASE_RADIUS = 0.006f;
constexpr float DEFAULT_DAMPING = 0.995f;
constexpr float WALL_BOUNCE = 0.72f;
constexpr float FIXED_DT = 0.016f;
constexpr float MIN_PARTICLE_SPEED = 0.015f;
constexpr float MAX_PARTICLE_SPEED = 0.32f;

// Configuración del bloque verde
constexpr float GREEN_GREEN_SPEEDUP = 1.02f;
constexpr float GREEN_BLUE_SPEEDUP = 1.04f;
constexpr float GREEN_RED_SPEEDUP = 1.08f;

// Configuración del bloque azul
constexpr float BLUE_VELOCITY_TRANSFER = 0.04f;
constexpr float BLUE_COLLISION_DAMPING = 0.92f;
constexpr float BLUE_NORMAL_PUSH = 0.006f;

// Configuración del bloque rojo
constexpr float RED_GREEN_RESPONSE = 0.35f;
constexpr float RED_BLUE_RESPONSE = 0.50f;
constexpr float RED_RED_RESPONSE = 0.20f;

constexpr int EMIT_COUNT = 1; // Cantidad de partículas emitidas por click
constexpr float EMIT_SPREAD = 0.035f;
constexpr float EMIT_SPEED = 0.22f; 

constexpr int WARMUP_FRAMES = 30;
constexpr int MEASURE_FRAMES = 300;
constexpr int LOG_EVERY_FRAMES = 60; // Cada cuantos frames se imprime el promedio de FPS en consola
} // namespace Config
