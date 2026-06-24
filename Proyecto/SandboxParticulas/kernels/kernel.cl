#define MATERIAL_RED 0
#define MATERIAL_GREEN 1
#define MATERIAL_BLUE 2

#define STATE_NORMAL 0
#define STATE_ALTERED 1

typedef struct {
    float4 pos_radius;
    float4 vel_misc;
    int type;
    int state;
    float energy;
    int padding;
} Particle;

typedef struct {
    float x;
    float y;
    float radius;
    int type;
    float energy;
} RenderParticle;

__kernel void update_particles_naive(
    __global const Particle* inParticles,
    __global Particle* outParticles,
    const float dt
) {
    const int i = get_global_id(0);
    if (i >= PARTICLE_COUNT) {
        return;
    }

    Particle p = inParticles[i];

    float2 pos = (float2)(p.pos_radius.x, p.pos_radius.y);
    float2 vel = (float2)(p.vel_misc.x, p.vel_misc.y);
    const float radius = p.pos_radius.z;

    for (int j = 0; j < PARTICLE_COUNT; ++j) {
        if (i == j) {
            continue;
        }

        const Particle q = inParticles[j];
        const float2 qpos = (float2)(q.pos_radius.x, q.pos_radius.y);
        const float2 delta = pos - qpos;
        const float dist2 = dot(delta, delta);
        const float minDist = radius + q.pos_radius.z;

        if (dist2 > 0.000001f && dist2 < minDist * minDist) {
            const float invDist = rsqrt(dist2);
            const float2 normal = delta * invDist;

            vel += normal * 0.02f;

            if (p.type == MATERIAL_RED && q.type == MATERIAL_GREEN) {
                const float oldVx = vel.x;
                vel.x = -vel.y * 1.20f;
                vel.y = oldVx * 1.20f;
                p.energy = 1.0f;
                p.state = STATE_ALTERED;
            } else if (p.type == MATERIAL_GREEN && q.type == MATERIAL_BLUE) {
                vel *= 0.92f;
                p.energy = 0.5f;
            } else if (p.type == MATERIAL_BLUE && q.type == MATERIAL_RED) {
                vel += normal * 0.03f;
                p.energy = 1.0f;
                p.state = STATE_ALTERED;
            }
        }
    }

    pos += vel * dt;
    vel *= p.vel_misc.z;

    if (pos.x < WORLD_MIN_X + radius) {
        pos.x = WORLD_MIN_X + radius;
        vel.x *= -WALL_BOUNCE;
    }
    if (pos.x > WORLD_MAX_X - radius) {
        pos.x = WORLD_MAX_X - radius;
        vel.x *= -WALL_BOUNCE;
    }
    if (pos.y < WORLD_MIN_Y + radius) {
        pos.y = WORLD_MIN_Y + radius;
        vel.y *= -WALL_BOUNCE;
    }
    if (pos.y > WORLD_MAX_Y - radius) {
        pos.y = WORLD_MAX_Y - radius;
        vel.y *= -WALL_BOUNCE;
    }

    p.pos_radius.x = pos.x;
    p.pos_radius.y = pos.y;
    p.vel_misc.x = vel.x;
    p.vel_misc.y = vel.y;
    p.energy = fmax(0.0f, p.energy - dt);
    if (p.energy <= 0.0f) {
        p.state = STATE_NORMAL;
    }

    outParticles[i] = p;
}

__kernel void build_render_particles(
    __global const Particle* particles,
    __global RenderParticle* renderParticles
) {
    const int i = get_global_id(0);
    if (i >= PARTICLE_COUNT) {
        return;
    }

    const Particle p = particles[i];
    renderParticles[i].x = p.pos_radius.x;
    renderParticles[i].y = p.pos_radius.y;
    renderParticles[i].radius = p.pos_radius.z;
    renderParticles[i].type = p.type;
    renderParticles[i].energy = p.energy;
}

