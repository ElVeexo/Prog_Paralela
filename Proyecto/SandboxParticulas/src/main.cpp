#include "config.hpp"
#include "gl_utils.hpp"
#include "opencl_utils.hpp"
#include "particles.hpp"
#include "render_utils.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <random>
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

std::string make_opencl_build_options() {
    return " -D PARTICLE_COUNT=" + std::to_string(Config::PARTICLE_COUNT) +
           " -D WORLD_MIN_X=" + std::to_string(Config::WORLD_MIN_X) + "f" +
           " -D WORLD_MAX_X=" + std::to_string(Config::WORLD_MAX_X) + "f" +
           " -D WORLD_MIN_Y=" + std::to_string(Config::WORLD_MIN_Y) + "f" +
           " -D WORLD_MAX_Y=" + std::to_string(Config::WORLD_MAX_Y) + "f" +
           " -D WALL_BOUNCE=" + std::to_string(Config::WALL_BOUNCE) + "f";
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

void write_emission_to_gpu(GLFWwindow* window,
                           cl_command_queue queue,
                           cl_mem dParticles,
                           int& emitCursor,
                           int selectedMaterial,
                           unsigned int& seed) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS) {
        return;
    }

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

    cl_int err = clEnqueueWriteBuffer(
        queue,
        dParticles,
        CL_FALSE,
        sizeof(Particle) * static_cast<std::size_t>(start),
        sizeof(Particle) * static_cast<std::size_t>(firstCount),
        emitted.data(),
        0,
        nullptr,
        nullptr
    );
    check_cl(err, "clEnqueueWriteBuffer emision primer bloque");

    if (secondCount > 0) {
        err = clEnqueueWriteBuffer(
            queue,
            dParticles,
            CL_FALSE,
            0,
            sizeof(Particle) * static_cast<std::size_t>(secondCount),
            emitted.data() + firstCount,
            0,
            nullptr,
            nullptr
        );
        check_cl(err, "clEnqueueWriteBuffer emision segundo bloque");
    }

    emitCursor = (emitCursor + Config::EMIT_COUNT) % Config::PARTICLE_COUNT;
}

int main() {
    GLFWwindow* window = init_window("Sandbox GPU OpenCL");
    if (window == nullptr) {
        return 1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    const unsigned int shaderProgram =
        create_shader_program("shaders/particles.vert", "shaders/particles.frag");
    const RenderObjects renderObjects = create_render_objects(shaderProgram);

    cl_int err = CL_SUCCESS;
    cl_platform_id platform = nullptr;
    cl_device_id device = nullptr;

    check_cl(clGetPlatformIDs(1, &platform, nullptr), "clGetPlatformIDs");
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
    if (err != CL_SUCCESS) {
        std::cout << "[INFO] No se encontro GPU OpenCL, usando CPU OpenCL como fallback." << std::endl;
        check_cl(clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, nullptr),
                 "clGetDeviceIDs CPU fallback");
    }

    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    check_cl(err, "clCreateContext");

    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, nullptr, &err);
    check_cl(err, "clCreateCommandQueueWithProperties");

    const std::string kernelSource = leer_kernel("kernels/kernel.cl");
    const char* sourcePtr = kernelSource.c_str();
    const size_t sourceSize = kernelSource.size();
    cl_program program = clCreateProgramWithSource(context, 1, &sourcePtr, &sourceSize, &err);
    check_cl(err, "clCreateProgramWithSource");

    const std::string buildOptions = make_opencl_build_options();
    err = clBuildProgram(program, 1, &device, buildOptions.c_str(), nullptr, nullptr);
    if (err != CL_SUCCESS) {
        print_build_log(program, device);
        check_cl(err, "clBuildProgram");
    }

    cl_kernel kUpdate = clCreateKernel(program, "update_particles_naive", &err);
    check_cl(err, "clCreateKernel update_particles_naive");
    cl_kernel kBuildRender = clCreateKernel(program, "build_render_particles", &err);
    check_cl(err, "clCreateKernel build_render_particles");

    unsigned int seed = 1234;
    std::vector<Particle> initialParticles = crear_particulas(Config::PARTICLE_COUNT, seed++);
    std::vector<RenderParticle> renderParticles(Config::PARTICLE_COUNT);

    cl_mem dParticlesIn = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        sizeof(Particle) * Config::PARTICLE_COUNT,
        initialParticles.data(),
        &err
    );
    check_cl(err, "clCreateBuffer dParticlesIn");

    cl_mem dParticlesOut = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE,
        sizeof(Particle) * Config::PARTICLE_COUNT,
        nullptr,
        &err
    );
    check_cl(err, "clCreateBuffer dParticlesOut");

    cl_mem dRenderParticles = clCreateBuffer(
        context,
        CL_MEM_WRITE_ONLY,
        sizeof(RenderParticle) * Config::PARTICLE_COUNT,
        nullptr,
        &err
    );
    check_cl(err, "clCreateBuffer dRenderParticles");

    size_t localSize = Config::LOCAL_SIZE;
    size_t globalSize =
        ((Config::PARTICLE_COUNT + localSize - 1) / localSize) * localSize;

    InputState input;
    FrameStats stats;
    int emitCursor = 0;

    while (!glfwWindowShouldClose(window)) {
        const auto frameStart = std::chrono::high_resolution_clock::now();

        process_input(window, input);

        if (input.resetRequested) {
            initialParticles = crear_particulas(Config::PARTICLE_COUNT, seed++);
            err = clEnqueueWriteBuffer(
                queue,
                dParticlesIn,
                CL_FALSE,
                0,
                sizeof(Particle) * Config::PARTICLE_COUNT,
                initialParticles.data(),
                0,
                nullptr,
                nullptr
            );
            check_cl(err, "clEnqueueWriteBuffer reset");
            emitCursor = 0;
            input.resetRequested = false;
        }

        write_emission_to_gpu(
            window,
            queue,
            dParticlesIn,
            emitCursor,
            input.selectedMaterial,
            seed
        );

        if (!input.paused) {
            err = clSetKernelArg(kUpdate, 0, sizeof(cl_mem), &dParticlesIn);
            check_cl(err, "clSetKernelArg update in");
            err = clSetKernelArg(kUpdate, 1, sizeof(cl_mem), &dParticlesOut);
            check_cl(err, "clSetKernelArg update out");
            const float dt = Config::FIXED_DT;
            err = clSetKernelArg(kUpdate, 2, sizeof(float), &dt);
            check_cl(err, "clSetKernelArg update dt");

            err = clEnqueueNDRangeKernel(
                queue,
                kUpdate,
                1,
                nullptr,
                &globalSize,
                &localSize,
                0,
                nullptr,
                nullptr
            );
            check_cl(err, "clEnqueueNDRangeKernel update");

            std::swap(dParticlesIn, dParticlesOut);
        }

        err = clSetKernelArg(kBuildRender, 0, sizeof(cl_mem), &dParticlesIn);
        check_cl(err, "clSetKernelArg render particles");
        err = clSetKernelArg(kBuildRender, 1, sizeof(cl_mem), &dRenderParticles);
        check_cl(err, "clSetKernelArg render output");

        err = clEnqueueNDRangeKernel(
            queue,
            kBuildRender,
            1,
            nullptr,
            &globalSize,
            &localSize,
            0,
            nullptr,
            nullptr
        );
        check_cl(err, "clEnqueueNDRangeKernel build_render_particles");

        err = clEnqueueReadBuffer(
            queue,
            dRenderParticles,
            CL_TRUE,
            0,
            sizeof(RenderParticle) * Config::PARTICLE_COUNT,
            renderParticles.data(),
            0,
            nullptr,
            nullptr
        );
        check_cl(err, "clEnqueueReadBuffer dRenderParticles");

        upload_render_particles(renderObjects, renderParticles);
        draw_particles(renderObjects);

        glfwSwapBuffers(window);
        glfwPollEvents();

        const auto frameEnd = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double, std::milli> frameMs = frameEnd - frameStart;
        update_stats(window, stats, "GPU", frameMs.count(), input.paused);
    }

    clReleaseMemObject(dParticlesIn);
    clReleaseMemObject(dParticlesOut);
    clReleaseMemObject(dRenderParticles);
    clReleaseKernel(kUpdate);
    clReleaseKernel(kBuildRender);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    destroy_render_objects(renderObjects);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

