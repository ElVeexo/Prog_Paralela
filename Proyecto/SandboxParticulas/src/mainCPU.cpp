#include "config.hpp"
#include "gl_utils.hpp"
#include "particles.hpp"
#include "render_utils.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

struct InputState {
    bool paused = false;
    bool lastSpace = false;
    bool lastR = false;
    bool resetRequested = false;
    int selectedMaterial = MATERIAL_RED;
};

struct FrameStats {
    int frame = 0;
    double accumulatedMs = 0.0;
};

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

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        input.selectedMaterial = MATERIAL_RED;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        input.selectedMaterial = MATERIAL_GREEN;
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        input.selectedMaterial = MATERIAL_BLUE;
    }
}

void maybe_emit_particles(GLFWwindow* window,
                          std::vector<Particle>& particles,
                          int& emitCursor,
                          int selectedMaterial,
                          unsigned int& seed) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS) {
        return;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    emit_particles(
        particles,
        emitCursor,
        screen_to_world_x(mouseX),
        screen_to_world_y(mouseY),
        selectedMaterial,
        seed++
    );
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

int main() {
    GLFWwindow* window = init_window("Sandbox CPU");
    if (window == nullptr) {
        return 1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    const unsigned int shaderProgram =
        create_shader_program("shaders/particles.vert", "shaders/particles.frag");
    const RenderObjects renderObjects = create_render_objects(shaderProgram);

    unsigned int seed = 1234;
    int emitCursor = 0;
    std::vector<Particle> particlesIn = crear_particulas(Config::PARTICLE_COUNT, seed++);
    std::vector<Particle> particlesOut = particlesIn;
    std::vector<RenderParticle> renderParticles(Config::PARTICLE_COUNT);

    InputState input;
    FrameStats stats;

    while (!glfwWindowShouldClose(window)) {
        const auto frameStart = std::chrono::high_resolution_clock::now();

        process_input(window, input);

        if (input.resetRequested) {
            particlesIn = crear_particulas(Config::PARTICLE_COUNT, seed++);
            particlesOut = particlesIn;
            emitCursor = 0;
            input.resetRequested = false;
        }

        maybe_emit_particles(window, particlesIn, emitCursor, input.selectedMaterial, seed);

        if (!input.paused) {
            update_cpu_naive(particlesIn, particlesOut, Config::FIXED_DT);
            std::swap(particlesIn, particlesOut);
        }

        build_render_particles_cpu(particlesIn, renderParticles);
        upload_render_particles(renderObjects, renderParticles);
        draw_particles(renderObjects);

        glfwSwapBuffers(window);
        glfwPollEvents();

        const auto frameEnd = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double, std::milli> frameMs = frameEnd - frameStart;
        update_stats(window, stats, "CPU", frameMs.count(), input.paused);
    }

    destroy_render_objects(renderObjects);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
