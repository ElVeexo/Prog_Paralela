#pragma once

#include "glad.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

inline std::string read_text_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "No se pudo abrir el archivo: " << path << std::endl;
        std::exit(1);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

inline unsigned int compile_shader(GLenum type, const char* source) {
    const unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    char infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        std::exit(1);
    }

    return shader;
}

inline unsigned int create_shader_program(const std::string& vertexPath,
                                          const std::string& fragmentPath) {
    const std::string vertexSource = read_text_file(vertexPath);
    const std::string fragmentSource = read_text_file(fragmentPath);

    const unsigned int vertexShader = compile_shader(GL_VERTEX_SHADER, vertexSource.c_str());
    const unsigned int fragmentShader = compile_shader(GL_FRAGMENT_SHADER, fragmentSource.c_str());

    const unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success = 0;
    char infoLog[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "ERROR::SHADER::LINKING_FAILED\n" << infoLog << std::endl;
        std::exit(1);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}
