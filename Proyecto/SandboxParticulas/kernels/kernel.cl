#define MATERIAL_RED 0
#define MATERIAL_GREEN 1
#define MATERIAL_BLUE 2

typedef struct {
    float x;
    float y;
    float radius;
    float vx;
    float vy;
    float damping;
    int type;
    float energy;
} Particle;

typedef struct {
    float x;
    float y;
    float radius;
    int type;
    float energy;
} RenderParticle;

void aplicar_interaccion_opencl(Particle* p, const Particle q) {
    const float2 pos = (float2)(p->x, p->y);
    const float2 qpos = (float2)(q.x, q.y);
    const float2 delta = pos - qpos;
    const float dist2 = dot(delta, delta);
    const float minDist = p->radius + q.radius;

    if (dist2 <= 0.000001f || dist2 >= minDist * minDist) {
        return;
    }

    const float invDist = rsqrt(dist2);
    const float2 normal = delta * invDist;

    p->vx += normal.x * 0.02f;
    p->vy += normal.y * 0.02f;

    if (p->type == MATERIAL_RED && q.type == MATERIAL_GREEN) {
        const float oldVx = p->vx;
        p->vx = -p->vy * 1.20f;
        p->vy = oldVx * 1.20f;
        p->energy = 1.0f;
    } else if (p->type == MATERIAL_GREEN && q.type == MATERIAL_BLUE) {
        p->vx *= 0.92f;
        p->vy *= 0.92f;
        p->energy = 0.5f;
    } else if (p->type == MATERIAL_BLUE && q.type == MATERIAL_RED) {
        p->vx += normal.x * 0.03f;
        p->vy += normal.y * 0.03f;
        p->energy = 1.0f;
    }
}

void integrar_y_rebotar_opencl(Particle* p, const float dt) {
    p->x += p->vx * dt;
    p->y += p->vy * dt;
    p->vx *= p->damping;
    p->vy *= p->damping;

    const float radius = p->radius;

    if (p->x < WORLD_MIN_X + radius) {
        p->x = WORLD_MIN_X + radius;
        p->vx *= -WALL_BOUNCE;
    }
    if (p->x > WORLD_MAX_X - radius) {
        p->x = WORLD_MAX_X - radius;
        p->vx *= -WALL_BOUNCE;
    }
    if (p->y < WORLD_MIN_Y + radius) {
        p->y = WORLD_MIN_Y + radius;
        p->vy *= -WALL_BOUNCE;
    }
    if (p->y > WORLD_MAX_Y - radius) {
        p->y = WORLD_MAX_Y - radius;
        p->vy *= -WALL_BOUNCE;
    }

    p->energy = fmax(0.0f, p->energy - dt);
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
    renderParticles[i].x = p.x;
    renderParticles[i].y = p.y;
    renderParticles[i].radius = p.radius;
    renderParticles[i].type = p.type;
    renderParticles[i].energy = p.energy;
}
