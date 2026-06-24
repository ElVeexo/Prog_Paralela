#pragma once

namespace Config {
constexpr int WINDOW_WIDTH = 600;
constexpr int WINDOW_HEIGHT = 600;

constexpr int PARTICLE_COUNT = 128;
constexpr int LOCAL_SIZE = 256;

constexpr float WORLD_MIN_X = -1.0f;
constexpr float WORLD_MAX_X = 1.0f;
constexpr float WORLD_MIN_Y = -1.0f;
constexpr float WORLD_MAX_Y = 1.0f;

constexpr float BASE_RADIUS = 0.006f;
constexpr float DEFAULT_DAMPING = 0.995f;
constexpr float WALL_BOUNCE = 0.90f;
constexpr float FIXED_DT = 0.016f;

constexpr int EMIT_COUNT = 1;
constexpr float EMIT_SPREAD = 0.035f;
constexpr float EMIT_SPEED = 0.22f;

constexpr int WARMUP_FRAMES = 30;
constexpr int MEASURE_FRAMES = 300;
constexpr int LOG_EVERY_FRAMES = 60;
} // namespace Config
