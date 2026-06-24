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

void aplicar_interaccion_opencl(Particle* p, const Particle q) {
    const float2 pos = (float2)(p->pos_radius.x, p->pos_radius.y);
    const float2 qpos = (float2)(q.pos_radius.x, q.pos_radius.y);
    const float2 delta = pos - qpos;
    const float dist2 = dot(delta, delta);
    const float minDist = p->pos_radius.z + q.pos_radius.z;

    if (dist2 <= 0.000001f || dist2 >= minDist * minDist) {
        return;
    }

    const float invDist = rsqrt(dist2);
    const float2 normal = delta * invDist;

    p->vel_misc.x += normal.x * 0.02f;
    p->vel_misc.y += normal.y * 0.02f;

    if (p->type == MATERIAL_RED && q.type == MATERIAL_GREEN) {
        const float oldVx = p->vel_misc.x;
        p->vel_misc.x = -p->vel_misc.y * 1.20f;
        p->vel_misc.y = oldVx * 1.20f;
        p->energy = 1.0f;
        p->state = STATE_ALTERED;
    } else if (p->type == MATERIAL_GREEN && q.type == MATERIAL_BLUE) {
        p->vel_misc.x *= 0.92f;
        p->vel_misc.y *= 0.92f;
        p->energy = 0.5f;
    } else if (p->type == MATERIAL_BLUE && q.type == MATERIAL_RED) {
        p->vel_misc.x += normal.x * 0.03f;
        p->vel_misc.y += normal.y * 0.03f;
        p->energy = 1.0f;
        p->state = STATE_ALTERED;
    }
}

void integrar_y_rebotar_opencl(Particle* p, const float dt) {
    p->pos_radius.x += p->vel_misc.x * dt;
    p->pos_radius.y += p->vel_misc.y * dt;
    p->vel_misc.x *= p->vel_misc.z;
    p->vel_misc.y *= p->vel_misc.z;

    const float radius = p->pos_radius.z;

    if (p->pos_radius.x < WORLD_MIN_X + radius) {
        p->pos_radius.x = WORLD_MIN_X + radius;
        p->vel_misc.x *= -WALL_BOUNCE;
    }
    if (p->pos_radius.x > WORLD_MAX_X - radius) {
        p->pos_radius.x = WORLD_MAX_X - radius;
        p->vel_misc.x *= -WALL_BOUNCE;
    }
    if (p->pos_radius.y < WORLD_MIN_Y + radius) {
        p->pos_radius.y = WORLD_MIN_Y + radius;
        p->vel_misc.y *= -WALL_BOUNCE;
    }
    if (p->pos_radius.y > WORLD_MAX_Y - radius) {
        p->pos_radius.y = WORLD_MAX_Y - radius;
        p->vel_misc.y *= -WALL_BOUNCE;
    }

    p->energy = fmax(0.0f, p->energy - dt);
    if (p->energy <= 0.0f) {
        p->state = STATE_NORMAL;
    }
}

__kernel void update_particles_tiled(
    __global const Particle* inParticles,
    __global Particle* outParticles,
    const float dt,
    __local Particle* tile
) {
    const int i = get_global_id(0);
    const int localId = get_local_id(0);
    const int localSize = get_local_size(0);
    const int active = i < PARTICLE_COUNT;

    Particle p;
    if (active) {
        p = inParticles[i];
    }

    for (int tileStart = 0; tileStart < PARTICLE_COUNT; tileStart += localSize) {
        const int j = tileStart + localId;

        if (j < PARTICLE_COUNT) {
            tile[localId] = inParticles[j];
        }

        barrier(CLK_LOCAL_MEM_FENCE);

        const int tileCount = min(localSize, PARTICLE_COUNT - tileStart);
        if (active) {
            for (int k = 0; k < tileCount; ++k) {
                const int globalJ = tileStart + k;
                if (globalJ == i) {
                    continue;
                }
                aplicar_interaccion_opencl(&p, tile[k]);
            }
        }

        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (active) {
        integrar_y_rebotar_opencl(&p, dt);
        outParticles[i] = p;
    }
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
