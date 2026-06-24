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

enum ParticleState {
    STATE_NORMAL = 0,
    STATE_ALTERED = 1
};

struct Float4 {
    float x;
    float y;
    float z;
    float w;
};

struct Particle {
    Float4 pos_radius;
    Float4 vel_misc;
    std::int32_t type;
    std::int32_t state;
    float energy;
    std::int32_t padding;
};

struct RenderParticle {
    float x;
    float y;
    float radius;
    std::int32_t type;
    float energy;
};

static_assert(sizeof(Float4) == 16, "Float4 must be 16 bytes");
static_assert(sizeof(Particle) == 48, "Particle layout must match OpenCL");
static_assert(sizeof(RenderParticle) == 20, "RenderParticle layout must match OpenCL");

inline float clamp_float(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

inline Particle make_particle(float x, float y, float vx, float vy, int type) {
    Particle p{};
    p.pos_radius = {x, y, Config::BASE_RADIUS, 0.0f};
    p.vel_misc = {vx, vy, Config::DEFAULT_DAMPING, 0.0f};
    p.type = type;
    p.state = STATE_NORMAL;
    p.energy = 0.0f;
    p.padding = 0;
    return p;
}

inline std::vector<Particle> crear_particulas(int count, unsigned int seed) {
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

inline void aplicar_interaccion_cpu(Particle& p, const Particle& q) {
    const float dx = p.pos_radius.x - q.pos_radius.x;
    const float dy = p.pos_radius.y - q.pos_radius.y;
    const float dist2 = dx * dx + dy * dy;
    const float minDist = p.pos_radius.z + q.pos_radius.z;

    if (dist2 <= 0.000001f || dist2 >= minDist * minDist) {
        return;
    }

    const float invDist = 1.0f / std::sqrt(dist2);
    const float nx = dx * invDist;
    const float ny = dy * invDist;

    p.vel_misc.x += nx * 0.02f;
    p.vel_misc.y += ny * 0.02f;

    if (p.type == MATERIAL_RED && q.type == MATERIAL_GREEN) {
        const float oldVx = p.vel_misc.x;
        p.vel_misc.x = -p.vel_misc.y * 1.20f;
        p.vel_misc.y = oldVx * 1.20f;
        p.energy = 1.0f;
        p.state = STATE_ALTERED;
    } else if (p.type == MATERIAL_GREEN && q.type == MATERIAL_BLUE) {
        p.vel_misc.x *= 0.92f;
        p.vel_misc.y *= 0.92f;
        p.energy = 0.5f;
    } else if (p.type == MATERIAL_BLUE && q.type == MATERIAL_RED) {
        p.vel_misc.x += nx * 0.03f;
        p.vel_misc.y += ny * 0.03f;
        p.energy = 1.0f;
        p.state = STATE_ALTERED;
    }
}

inline void integrar_y_rebotar_cpu(Particle& p, float dt) {
    p.pos_radius.x += p.vel_misc.x * dt;
    p.pos_radius.y += p.vel_misc.y * dt;
    p.vel_misc.x *= p.vel_misc.z;
    p.vel_misc.y *= p.vel_misc.z;

    const float radius = p.pos_radius.z;

    if (p.pos_radius.x < Config::WORLD_MIN_X + radius) {
        p.pos_radius.x = Config::WORLD_MIN_X + radius;
        p.vel_misc.x *= -Config::WALL_BOUNCE;
    }
    if (p.pos_radius.x > Config::WORLD_MAX_X - radius) {
        p.pos_radius.x = Config::WORLD_MAX_X - radius;
        p.vel_misc.x *= -Config::WALL_BOUNCE;
    }
    if (p.pos_radius.y < Config::WORLD_MIN_Y + radius) {
        p.pos_radius.y = Config::WORLD_MIN_Y + radius;
        p.vel_misc.y *= -Config::WALL_BOUNCE;
    }
    if (p.pos_radius.y > Config::WORLD_MAX_Y - radius) {
        p.pos_radius.y = Config::WORLD_MAX_Y - radius;
        p.vel_misc.y *= -Config::WALL_BOUNCE;
    }

    p.energy = std::max(0.0f, p.energy - dt);
    if (p.energy <= 0.0f) {
        p.state = STATE_NORMAL;
    }
}

inline void update_cpu_naive(const std::vector<Particle>& in,
                             std::vector<Particle>& out,
                             float dt) {
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
    const std::size_t count = particles.size();
    renderParticles.resize(count);

    for (std::size_t i = 0; i < count; ++i) {
        renderParticles[i] = {
            particles[i].pos_radius.x,
            particles[i].pos_radius.y,
            particles[i].pos_radius.z,
            particles[i].type,
            particles[i].energy
        };
    }
}

inline float screen_to_world_x(double mouseX) {
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

