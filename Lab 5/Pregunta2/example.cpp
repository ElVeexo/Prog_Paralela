#include <iostream>
#include <cmath>

// Incluir GLAD antes de GLFW
#include "glad.h"
#include <GLFW/glfw3.h>

// Variables globales para la animación
float offset_x = 0.0f;
float offset_y = 0.0f;
const float velocidad = 2.0f; 
float tiempo_inicio = 0.0f;

// Bandera para controlar la pulsación única de la tecla C
bool c_presionada = false;

// Solo necesitamos un punto central en el origen (0,0,0) ya que los offsets harán el movimiento
float vertices[] = {
    0.0f, 0.0f, 0.0f
};

// Vértice Shader modificado para aceptar offsets en ambos ejes X e Y
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform float uOffsetX;\n" 
    "uniform float uOffsetY;\n" 
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x + uOffsetX, aPos.y + uOffsetY, aPos.z, 1.0);\n"
    "}\0";

// Fragment Shader modificado para recortar el cuadrado y volverlo un círculo
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform float uColorBlue;\n"
    "uniform float uColorGreen;\n"
    "uniform float uColorRed;\n"
    "void main()\n"
    "{\n"
    "   // gl_PointCoord va de (0,0) a (1,1) dentro del punto.\n"
    "   // Calculamos la distancia al centro (0.5, 0.5)\n"
    "   vec2 centro = gl_PointCoord - vec2(0.5);\n"
    "   if (dot(centro, centro) > 0.25) {\n" // 0.25 es el radio 0.5 al cuadrado
    "       discard;\n"                      // Descarta los píxeles de las esquinas
    "   }\n"
    "   FragColor = vec4(1.0f, 0.5f, uColorBlue, 1.0f);\n"
    "   FragColor = vec4(uColorRed, uColorGreen, uColorBlue, 1.0f);\n"

    "}\0";


// Fragment Shader modificado para recortar el cuadrado y volverlo un círculo
const char *fragmentShaderSource_circle = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform float uColorBlue;\n"
    "uniform float uColorGreen;\n"
    "uniform float uColorRed;\n"
    "void main()\n"
    "{\n"
    "   // gl_PointCoord va de (0,0) a (1,1) dentro del punto.\n"
    "   // Calculamos la distancia al centro (0.5, 0.5)\n"
    "   FragColor = vec4(1.0f, 0.5f, uColorBlue, 1.0f);\n"
    "   FragColor = vec4(uColorRed, uColorGreen, uColorBlue, 1.0f);\n"
    "}\0";

// Prototipos de funciones
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
unsigned int compileShader(GLenum type, const char* source);
unsigned int createShaderProgram(const char* vsSource, const char* fsSource);


int main() {
    // 1. Inicialización de GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Creación de la Ventana
    GLFWwindow* window = glfwCreateWindow(600, 600, "Ruta Ovalada con Circulo", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 3. Cargar Funciones de OpenGL (GLAD)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // 4. Configurar Shaders
    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    unsigned int shaderProgram_circle = createShaderProgram(vertexShaderSource, fragmentShaderSource_circle);


    // 5. Configurar VBO y VAO
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);
    
    tiempo_inicio = glfwGetTime();

    // 6. Bucle de Renderizado Principal
    while (!glfwWindowShouldClose(window)) {
        // Entrada del usuario e impresión por terminal
        processInput(window);

        // Lógica de la animación de la ruta ovalada
        float tiempo_actual = glfwGetTime();
        float t = velocidad * (tiempo_actual - tiempo_inicio);
        
        // Ecuaciones paramétricas de la elipse/óvalo (Amplitudes distintas en X e Y)
        offset_x = 0.6f * cos(t);
        offset_y = 0.3f * sin(t);
        
        // Dinámica de color basada en la posición X
        float color_blue = (offset_x + 0.6f) / 1.2f;
        float color_green = 0.0f;
        float color_red = 0.0f; 
        //printf("x: %.2f, y: %.2f, color_blue: %.2f\n", offset_x, offset_y, color_blue);
        if (offset_x < 0 && offset_y > 0) {
            color_red = 0.0f;
            color_green = 0.0f; 
            color_blue = 1.0f; //Solo Azul
        } else if (offset_x < 0 && offset_y < 0) {
            color_red = 1.0f; // Solo rojo
            color_green = 0.0f; 
            color_blue = 0.0f; 
        } else if (offset_x > 0 && offset_y < 0) {
            color_red = 0.0f;
            color_green = 1.0f; // Solo verde
            color_blue = 0.1f;
        } else {
            color_red = 0.3f;
            color_green = 0.3f; // Rojo, verde y azul
            color_blue = 0.4f;
        }

        // Renderizado
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Pasar de circulo a cuadrado cuando se presiona la tecla c
        if (c_presionada == true){
            glUseProgram(shaderProgram);
        }
        else {
            glUseProgram(shaderProgram_circle);
        }
        
        // Pasar offsets a los Uniforms de la GPU
        int xLocation = glGetUniformLocation(shaderProgram, "uOffsetX");
        glUniform1f(xLocation, offset_x);

        int yLocation = glGetUniformLocation(shaderProgram, "uOffsetY");
        glUniform1f(yLocation, offset_y);

        int colorLocation = glGetUniformLocation(shaderProgram, "uColorBlue");
        glUniform1f(colorLocation, color_blue);

        int colorGreenLocation = glGetUniformLocation(shaderProgram, "uColorGreen");
        glUniform1f(colorGreenLocation, color_green);

        int colorRedLocation = glGetUniformLocation(shaderProgram, "uColorRed");
        glUniform1f(colorRedLocation, color_red);

        // Habilitar el tamaño de punto mapeado y dibujar
        glPointSize(60); 
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, 1);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 7. Limpieza
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}


// --- Implementación de Funciones Auxiliares ---

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Función modificada para procesar la tecla Escape y la tecla C de forma limpia
void processInput(GLFWwindow *window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Sistema de validación por flanco (Evita spam en la terminal al dejar apretado)
    if(glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if(!c_presionada) {
            std::cout << "[INFO] Tecla 'C' presionada. Coordenadas del circulo -> X: " 
                      << offset_x << " | Y: " << offset_y << std::endl;
            
            c_presionada = true; // Bloquea hasta que suelte la tecla
        }
    }
    if(glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
        c_presionada = false; // Libera el bloqueo
    }
}

unsigned int compileShader(GLenum type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

unsigned int createShaderProgram(const char* vsSource, const char* fsSource) {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vsSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}