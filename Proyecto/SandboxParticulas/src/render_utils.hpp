#pragma once

#include "config.hpp"
#include "glad.h"
#include "particles.hpp"

#include <GLFW/glfw3.h>

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

struct RenderObjects {
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int shader = 0;
};

inline GLFWwindow* init_window(const char* title) {
    if (!glfwInit()) {
        std::cerr << "No se pudo inicializar GLFW" << std::endl;
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        Config::WINDOW_WIDTH,
        Config::WINDOW_HEIGHT,
        title,
        nullptr,
        nullptr
    );

    if (window == nullptr) {
        std::cerr << "No se pudo crear la ventana GLFW" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "No se pudo inicializar GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return nullptr;
    }

    glViewport(0, 0, Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return window;
}

inline void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

inline RenderObjects create_render_objects(unsigned int shaderProgram) {
    RenderObjects objects{};
    objects.shader = shaderProgram;

    glGenVertexArrays(1, &objects.vao);
    glGenBuffers(1, &objects.vbo);

    glBindVertexArray(objects.vao);
    glBindBuffer(GL_ARRAY_BUFFER, objects.vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(RenderParticle) * Config::PARTICLE_COUNT,
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(RenderParticle),
        reinterpret_cast<void*>(offsetof(RenderParticle, x))
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(RenderParticle),
        reinterpret_cast<void*>(offsetof(RenderParticle, radius))
    );
    glEnableVertexAttribArray(1);

    glVertexAttribIPointer(
        2,
        1,
        GL_INT,
        sizeof(RenderParticle),
        reinterpret_cast<void*>(offsetof(RenderParticle, type))
    );
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(
        3,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(RenderParticle),
        reinterpret_cast<void*>(offsetof(RenderParticle, energy))
    );
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return objects;
}

inline void upload_render_particles(const RenderObjects& objects,
                                    const std::vector<RenderParticle>& renderParticles) {
    glBindBuffer(GL_ARRAY_BUFFER, objects.vbo);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(RenderParticle) * renderParticles.size(),
        renderParticles.data()
    );
}

inline void draw_particles(const RenderObjects& objects) {
    glClearColor(0.05f, 0.06f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(objects.shader);
    const int scaleLocation = glGetUniformLocation(objects.shader, "uPointScale");
    glUniform1f(scaleLocation, static_cast<float>(Config::WINDOW_HEIGHT));

    glBindVertexArray(objects.vao);
    glDrawArrays(GL_POINTS, 0, Config::PARTICLE_COUNT);
}

inline void destroy_render_objects(const RenderObjects& objects) {
    glDeleteVertexArrays(1, &objects.vao);
    glDeleteBuffers(1, &objects.vbo);
    glDeleteProgram(objects.shader);
}
