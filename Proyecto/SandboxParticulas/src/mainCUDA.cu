#include "config.hpp"
#include "gl_utils.hpp"
#include "particles.hpp"
#include "render_utils.hpp"

#include <GLFW/glfw3.h>
#include <cuda_runtime.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

struct InputState {
    bool paused = false;
    bool lastSpace = false;
    bool lastR = false;
    bool last1 = false;
    bool last2 = false;
    bool last3 = false;
    bool lastLeftMouse = false;
    bool resetRequested = false;
    int selectedMaterial = MATERIAL_RED;
};

struct FrameStats {
    int frame = 0;
    double accumulatedMs = 0.0;
};

void check_cuda(cudaError_t err, const char* operation) {
    if (err != cudaSuccess) {
        std::cerr << "CUDA error en " << operation << ": "
                  << cudaGetErrorString(err) << std::endl;
        std::exit(1);
    }
}

const char* material_name(int material) {
    if (material == MATERIAL_RED) {
        return "rojo";
    }
    if (material == MATERIAL_GREEN) {
        return "verde";
    }
    return "azul";
}

void select_material(InputState& input, int material) {
    if (input.selectedMaterial == material) {
        return;
    }

    input.selectedMaterial = material;
    std::cout << "[INFO] Color seleccionado: " << material_name(material) << std::endl;
}

void process_input(GLFWwindow* window, InputState& input) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    const bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (spacePressed && !input.lastSpace) {
        input.paused = !input.paused;
    }
    input.lastSpace = spacePressed;

    const bool rPressed = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    if (rPressed && !input.lastR) {
        input.resetRequested = true;
    }
    input.lastR = rPressed;

    const bool key1Pressed = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
    if (key1Pressed && !input.last1) {
        select_material(input, MATERIAL_RED);
    }
    input.last1 = key1Pressed;

    const bool key2Pressed = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
    if (key2Pressed && !input.last2) {
        select_material(input, MATERIAL_GREEN);
    }
    input.last2 = key2Pressed;

    const bool key3Pressed = glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS;
    if (key3Pressed && !input.last3) {
        select_material(input, MATERIAL_BLUE);
    }
    input.last3 = key3Pressed;
}

void update_stats(GLFWwindow* window,
                  FrameStats& stats,
                  const char* label,
                  double frameMs,
                  bool paused) {
    stats.frame++;

    if (stats.frame > Config::WARMUP_FRAMES) {
        stats.accumulatedMs += frameMs;
    }

    if (stats.frame % Config::LOG_EVERY_FRAMES != 0) {
        return;
    }

    const int measuredFrames = std::max(1, stats.frame - Config::WARMUP_FRAMES);
    const double avgMs = stats.accumulatedMs / measuredFrames;
    const double fps = 1000.0 / std::max(0.0001, avgMs);

    char title[256];
    std::snprintf(
        title,
        sizeof(title),
        "%s | N=%d | %.2f ms | %.1f FPS%s",
        label,
        Config::PARTICLE_COUNT,
        avgMs,
        fps,
        paused ? " | PAUSA" : ""
    );
    glfwSetWindowTitle(window, title);

    std::cout << label << " | N=" << Config::PARTICLE_COUNT
              << " | frame=" << avgMs << " ms"
              << " | FPS=" << fps
              << (paused ? " | PAUSA" : "")
              << std::endl;
}

std::vector<Particle> crear_bloque_emision(float x,
                                           float y,
                                           int material,
                                           unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> spread(-Config::EMIT_SPREAD, Config::EMIT_SPREAD);
    std::uniform_real_distribution<float> speed(-Config::EMIT_SPEED, Config::EMIT_SPEED);

    std::vector<Particle> emitted;
    emitted.reserve(Config::EMIT_COUNT);

    for (int i = 0; i < Config::EMIT_COUNT; ++i) {
        emitted.push_back(make_particle(
            clamp_float(x + spread(rng), Config::WORLD_MIN_X + 0.02f, Config::WORLD_MAX_X - 0.02f),
            clamp_float(y + spread(rng), Config::WORLD_MIN_Y + 0.02f, Config::WORLD_MAX_Y - 0.02f),
            speed(rng),
            speed(rng),
            material
        ));
    }

    return emitted;
}

void write_emission_to_cuda(GLFWwindow* window,
                            Particle* dParticles,
                            int& emitCursor,
                            int selectedMaterial,
                            bool& lastLeftMouse,
                            unsigned int& seed) {
    const bool leftMousePressed =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    if (!leftMousePressed) {
        lastLeftMouse = false;
        return;
    }
    if (lastLeftMouse) {
        return;
    }
    lastLeftMouse = true;

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    const std::vector<Particle> emitted = crear_bloque_emision(
        screen_to_world_x(mouseX),
        screen_to_world_y(mouseY),
        selectedMaterial,
        seed++
    );

    const int start = emitCursor;
    const int firstCount = std::min(Config::EMIT_COUNT, Config::PARTICLE_COUNT - start);
    const int secondCount = Config::EMIT_COUNT - firstCount;

    check_cuda(
        cudaMemcpy(
            dParticles + start,
            emitted.data(),
            sizeof(Particle) * static_cast<std::size_t>(firstCount),
            cudaMemcpyHostToDevice
        ),
        "cudaMemcpy emision primer bloque"
    );

    if (secondCount > 0) {
        check_cuda(
            cudaMemcpy(
                dParticles,
                emitted.data() + firstCount,
                sizeof(Particle) * static_cast<std::size_t>(secondCount),
                cudaMemcpyHostToDevice
            ),
            "cudaMemcpy emision segundo bloque"
        );
    }

    emitCursor = (emitCursor + Config::EMIT_COUNT) % Config::PARTICLE_COUNT;
}

__device__ void aplicar_interaccion_cuda(Particle& p, const Particle& q) {
    const float dx = p.pos_radius.x - q.pos_radius.x;
    const float dy = p.pos_radius.y - q.pos_radius.y;
    const float dist2 = dx * dx + dy * dy;
    const float minDist = p.pos_radius.z + q.pos_radius.z;

    if (dist2 <= 0.000001f || dist2 >= minDist * minDist) {
        return;
    }

    const float invDist = rsqrtf(dist2);
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

__device__ void integrar_y_rebotar_cuda(Particle& p, float dt) {
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

    p.energy = fmaxf(0.0f, p.energy - dt);
    if (p.energy <= 0.0f) {
        p.state = STATE_NORMAL;
    }
}

__global__ void update_particles_tiled_cuda(const Particle* inParticles,
                                            Particle* outParticles,
                                            float dt) {
    __shared__ Particle tile[Config::LOCAL_SIZE];

    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    const int localId = threadIdx.x;
    const bool active = i < Config::PARTICLE_COUNT;

    Particle p;
    if (active) {
        p = inParticles[i];
    }

    for (int tileStart = 0; tileStart < Config::PARTICLE_COUNT; tileStart += blockDim.x) {
        const int j = tileStart + localId;

        if (j < Config::PARTICLE_COUNT) {
            tile[localId] = inParticles[j];
        }

        __syncthreads();

        const int tileCount = min(blockDim.x, Config::PARTICLE_COUNT - tileStart);
        if (active) {
            for (int k = 0; k < tileCount; ++k) {
                const int globalJ = tileStart + k;
                if (globalJ == i) {
                    continue;
                }
                aplicar_interaccion_cuda(p, tile[k]);
            }
        }

        __syncthreads();
    }

    if (active) {
        integrar_y_rebotar_cuda(p, dt);
        outParticles[i] = p;
    }
}

__global__ void build_render_particles_cuda(const Particle* particles,
                                            RenderParticle* renderParticles) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= Config::PARTICLE_COUNT) {
        return;
    }

    const Particle p = particles[i];
    renderParticles[i].x = p.pos_radius.x;
    renderParticles[i].y = p.pos_radius.y;
    renderParticles[i].radius = p.pos_radius.z;
    renderParticles[i].type = p.type;
    renderParticles[i].energy = p.energy;
}

int main() {
    GLFWwindow* window = init_window("Sandbox GPU CUDA");
    if (window == nullptr) {
        return 1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    const unsigned int shaderProgram =
        create_shader_program("shaders/particles.vert", "shaders/particles.frag");
    const RenderObjects renderObjects = create_render_objects(shaderProgram);

    unsigned int seed = 1234;
    std::vector<Particle> initialParticles = crear_particulas(Config::PARTICLE_COUNT, seed++);
    std::vector<RenderParticle> renderParticles(Config::PARTICLE_COUNT);

    Particle* dParticlesIn = nullptr;
    Particle* dParticlesOut = nullptr;
    RenderParticle* dRenderParticles = nullptr;

    check_cuda(cudaMalloc(&dParticlesIn, sizeof(Particle) * Config::PARTICLE_COUNT),
               "cudaMalloc dParticlesIn");
    check_cuda(cudaMalloc(&dParticlesOut, sizeof(Particle) * Config::PARTICLE_COUNT),
               "cudaMalloc dParticlesOut");
    check_cuda(cudaMalloc(&dRenderParticles, sizeof(RenderParticle) * Config::PARTICLE_COUNT),
               "cudaMalloc dRenderParticles");

    check_cuda(
        cudaMemcpy(
            dParticlesIn,
            initialParticles.data(),
            sizeof(Particle) * Config::PARTICLE_COUNT,
            cudaMemcpyHostToDevice
        ),
        "cudaMemcpy particulas iniciales"
    );

    const int threads = Config::LOCAL_SIZE;
    const int blocks = (Config::PARTICLE_COUNT + threads - 1) / threads;

    InputState input;
    FrameStats stats;
    int emitCursor = 0;

    while (!glfwWindowShouldClose(window)) {
        const auto frameStart = std::chrono::high_resolution_clock::now();

        process_input(window, input);

        if (input.resetRequested) {
            initialParticles = crear_particulas(Config::PARTICLE_COUNT, seed++);
            check_cuda(
                cudaMemcpy(
                    dParticlesIn,
                    initialParticles.data(),
                    sizeof(Particle) * Config::PARTICLE_COUNT,
                    cudaMemcpyHostToDevice
                ),
                "cudaMemcpy reset"
            );
            emitCursor = 0;
            input.resetRequested = false;
        }

        write_emission_to_cuda(
            window,
            dParticlesIn,
            emitCursor,
            input.selectedMaterial,
            input.lastLeftMouse,
            seed
        );

        if (!input.paused) {
            update_particles_tiled_cuda<<<blocks, threads>>>(
                dParticlesIn,
                dParticlesOut,
                Config::FIXED_DT
            );
            check_cuda(cudaGetLastError(), "launch update_particles_tiled_cuda");
            check_cuda(cudaDeviceSynchronize(), "cudaDeviceSynchronize update");
            std::swap(dParticlesIn, dParticlesOut);
        }

        build_render_particles_cuda<<<blocks, threads>>>(dParticlesIn, dRenderParticles);
        check_cuda(cudaGetLastError(), "launch build_render_particles_cuda");
        check_cuda(cudaDeviceSynchronize(), "cudaDeviceSynchronize build render");

        check_cuda(
            cudaMemcpy(
                renderParticles.data(),
                dRenderParticles,
                sizeof(RenderParticle) * Config::PARTICLE_COUNT,
                cudaMemcpyDeviceToHost
            ),
            "cudaMemcpy renderParticles device to host"
        );

        upload_render_particles(renderObjects, renderParticles);
        draw_particles(renderObjects);

        glfwSwapBuffers(window);
        glfwPollEvents();

        const auto frameEnd = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double, std::milli> frameMs = frameEnd - frameStart;
        update_stats(window, stats, "CUDA", frameMs.count(), input.paused);
    }

    cudaFree(dParticlesIn);
    cudaFree(dParticlesOut);
    cudaFree(dRenderParticles);

    destroy_render_objects(renderObjects);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
