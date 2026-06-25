#pragma once

#include "config.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

enum MaterialType {
    MATERIAL_RED = 0,
    MATERIAL_GREEN = 1,
    MATERIAL_BLUE = 2
};

struct Particle {
    float x;
    float y;
    float radius;
    float vx;
    float vy;
    float damping;
    std::int32_t type;
    float energy;
};

struct RenderParticle {
    float x;
    float y;
    float radius;
    std::int32_t type;
    float energy;
};

static_assert(sizeof(Particle) == 32, "Particle layout must match CUDA/OpenCL");
static_assert(sizeof(RenderParticle) == 20, "RenderParticle layout must match OpenCL");

inline float clamp_float(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

inline Particle make_particle(float x, float y, float vx, float vy, int type) {
    Particle p{};
    p.x = x;
    p.y = y;
    p.radius = Config::BASE_RADIUS;
    p.vx = vx;
    p.vy = vy;
    p.damping = Config::DEFAULT_DAMPING;
    p.type = type;
    p.energy = 0.0f;
    return p;
}

inline std::vector<Particle> crear_particulas(int count, unsigned int seed) {
    // Crear partículas con posiciones y velocidades aleatorias dentro de los límites del mundo cuando inicia o se reinicia la simulación
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> posX(Config::WORLD_MIN_X + 0.06f,
                                               Config::WORLD_MAX_X - 0.06f);
    std::uniform_real_distribution<float> posY(Config::WORLD_MIN_Y + 0.06f,
                                               Config::WORLD_MAX_Y - 0.06f);
    std::uniform_real_distribution<float> vel(-0.18f, 0.18f);

    std::vector<Particle> particles;
    particles.reserve(static_cast<std::size_t>(count));

    for (int i = 0; i < count; ++i) {
        particles.push_back(make_particle(posX(rng), posY(rng), vel(rng), vel(rng), i % 3));
    }

    return particles;
}

inline void limitar_velocidad_cpu(Particle& p) {
    // Dado que se forma un cáos al colisionar las partículas, además de que aumentamos la cantidad
    // es ncesario limitar la velocidad de las partículas para que no se salgan de la pantalla y se mantenga un comportamiento estable y visible
    const float vx = p.vx;
    const float vy = p.vy;
    const float speed2 = vx * vx + vy * vy;
    const float minSpeed = Config::MIN_PARTICLE_SPEED;
    const float maxSpeed = Config::MAX_PARTICLE_SPEED;

    if (speed2 < minSpeed * minSpeed) {
        if (speed2 <= 0.000001f) {
            p.vx = minSpeed;
            p.vy = 0.0f;
            return;
        }

        const float invSpeed = 1.0f / std::sqrt(speed2);
        p.vx = vx * invSpeed * minSpeed;
        p.vy = vy * invSpeed * minSpeed;
        return;
    }

    if (speed2 > maxSpeed * maxSpeed) {
        const float invSpeed = 1.0f / std::sqrt(speed2);
        p.vx = vx * invSpeed * maxSpeed;
        p.vy = vy * invSpeed * maxSpeed;
    }
}

inline void aplicar_interaccion_cpu(Particle& p, const Particle& q) {
    // Definir el comportamiento de las partículas según sus materiales
    const float dx = p.x - q.x;
    const float dy = p.y - q.y;
    const float dist2 = dx * dx + dy * dy;
    const float minDist = p.radius + q.radius;

    if (dist2 <= 0.000001f || dist2 >= minDist * minDist) {
        return;
    }

    const float invDist = 1.0f / std::sqrt(dist2);
    const float nx = dx * invDist;
    const float ny = dy * invDist;

    if (p.type == MATERIAL_GREEN && q.type == MATERIAL_GREEN) {
        p.vx *= Config::GREEN_GREEN_SPEEDUP;
        p.vy *= Config::GREEN_GREEN_SPEEDUP;
        p.vx += nx * 0.02f;
        p.vy += ny * 0.02f;
        p.energy = 0.8f;
    } else if (p.type == MATERIAL_GREEN && q.type == MATERIAL_BLUE) {
        p.vx *= Config::GREEN_BLUE_SPEEDUP;
        p.vy *= Config::GREEN_BLUE_SPEEDUP;
        p.vx += nx * 0.02f;
        p.vy += ny * 0.02f;
        p.energy = 0.7f;
    } else if (p.type == MATERIAL_GREEN && q.type == MATERIAL_RED) {
        p.vx *= Config::GREEN_RED_SPEEDUP;
        p.vy *= Config::GREEN_RED_SPEEDUP;
        p.vx += nx * 0.03f;
        p.vy += ny * 0.03f;
        p.energy = 1.0f;
    } else if (p.type == MATERIAL_BLUE) {
        p.vx = p.vx * Config::BLUE_COLLISION_DAMPING +
               q.vx * Config::BLUE_VELOCITY_TRANSFER +
               nx * Config::BLUE_NORMAL_PUSH;
        p.vy = p.vy * Config::BLUE_COLLISION_DAMPING +
               q.vy * Config::BLUE_VELOCITY_TRANSFER +
               ny * Config::BLUE_NORMAL_PUSH;
        p.energy = 0.6f;
    } else if (p.type == MATERIAL_RED && q.type == MATERIAL_GREEN) {
        p.vx += nx * Config::RED_GREEN_RESPONSE * 0.02f;
        p.vy += ny * Config::RED_GREEN_RESPONSE * 0.02f;
        p.energy = 0.35f;
    } else if (p.type == MATERIAL_RED && q.type == MATERIAL_BLUE) {
        p.vx += nx * Config::RED_BLUE_RESPONSE * 0.02f;
        p.vy += ny * Config::RED_BLUE_RESPONSE * 0.02f;
        p.energy = 0.45f;
    } else if (p.type == MATERIAL_RED && q.type == MATERIAL_RED) {
        p.vx += nx * Config::RED_RED_RESPONSE * 0.02f;
        p.vy += ny * Config::RED_RED_RESPONSE * 0.02f;
        p.energy = 0.25f;
    }
}

inline void integrar_y_rebotar_cpu(Particle& p, float dt) {
    // Integrar la posición y aplicar rebotes en los límites del mundo
    p.x += p.vx * dt;
    p.y += p.vy * dt;
    p.vx *= p.damping;
    p.vy *= p.damping;

    const float radius = p.radius;

    if (p.x < Config::WORLD_MIN_X + radius) {
        p.x = Config::WORLD_MIN_X + radius;
        p.vx *= -Config::WALL_BOUNCE;
    }
    if (p.x > Config::WORLD_MAX_X - radius) {
        p.x = Config::WORLD_MAX_X - radius;
        p.vx *= -Config::WALL_BOUNCE;
    }
    if (p.y < Config::WORLD_MIN_Y + radius) {
        p.y = Config::WORLD_MIN_Y + radius;
        p.vy *= -Config::WALL_BOUNCE;
    }
    if (p.y > Config::WORLD_MAX_Y - radius) {
        p.y = Config::WORLD_MAX_Y - radius;
        p.vy *= -Config::WALL_BOUNCE;
    }

    p.energy = std::max(0.0f, p.energy - dt);

    limitar_velocidad_cpu(p);
}

inline void update_cpu_secuencial(const std::vector<Particle>& in,
                                  std::vector<Particle>& out,
                                  float dt) {
    // Bucle principal para actualizar las partículas de manera secuencial
    const int count = static_cast<int>(in.size());

    for (int i = 0; i < count; ++i) {
        Particle p = in[static_cast<std::size_t>(i)];

        for (int j = 0; j < count; ++j) {
            if (i == j) {
                continue;
            }
            aplicar_interaccion_cpu(p, in[static_cast<std::size_t>(j)]);
        }

        integrar_y_rebotar_cpu(p, dt);
        out[static_cast<std::size_t>(i)] = p;
    }
}

inline void build_render_particles_cpu(const std::vector<Particle>& particles,
                                       std::vector<RenderParticle>& renderParticles) {
    // Renderizar las partículas para la visualización
    const std::size_t count = particles.size();
    renderParticles.resize(count);

    for (std::size_t i = 0; i < count; ++i) {
        renderParticles[i] = {
            particles[i].x,
            particles[i].y,
            particles[i].radius,
            particles[i].type,
            particles[i].energy
        };
    }
}

inline float screen_to_world_x(double mouseX) {
    // Convertir las coordenadas de la pantalla a coordenadas del mundo (van del -1.0f al 1.0f)
    const float normalized = static_cast<float>(mouseX) / static_cast<float>(Config::WINDOW_WIDTH);
    return Config::WORLD_MIN_X + normalized * (Config::WORLD_MAX_X - Config::WORLD_MIN_X);
}

inline float screen_to_world_y(double mouseY) {
    const float normalized = static_cast<float>(mouseY) / static_cast<float>(Config::WINDOW_HEIGHT);
    return Config::WORLD_MAX_Y - normalized * (Config::WORLD_MAX_Y - Config::WORLD_MIN_Y);
}

inline void emit_particles(std::vector<Particle>& particles,
                           int& cursor,
                           float x,
                           float y,
                           int material,
                           unsigned int seed) {
    // Inicialización de generadores de números aleatorios para la dispersión y velocidad de las partículas emitidas
    if (particles.empty()) {
        return;
    }

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> spread(-Config::EMIT_SPREAD, Config::EMIT_SPREAD);
    std::uniform_real_distribution<float> speed(-Config::EMIT_SPEED, Config::EMIT_SPEED);

    const int count = static_cast<int>(particles.size());
    for (int i = 0; i < Config::EMIT_COUNT; ++i) {
        const int index = cursor % count;
        particles[static_cast<std::size_t>(index)] = make_particle(
            clamp_float(x + spread(rng), Config::WORLD_MIN_X + 0.02f, Config::WORLD_MAX_X - 0.02f),
            clamp_float(y + spread(rng), Config::WORLD_MIN_Y + 0.02f, Config::WORLD_MAX_Y - 0.02f),
            speed(rng),
            speed(rng),
            material
        );
        cursor = (cursor + 1) % count;
    }
}
