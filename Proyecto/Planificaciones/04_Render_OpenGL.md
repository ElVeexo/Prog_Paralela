# Render OpenGL Comun

## Alcance

Tanto `mainCPU.cpp` como `main.cpp` deben tener ventana OpenGL y render en tiempo real.

La idea es que ambos ejecutables se vean igual, pero difieran en quien actualiza la simulacion:

```text
mainCPU.cpp -> update en CPU
main.cpp    -> update en OpenCL
```

## Ventana

Resolucion inicial fija desde `config.hpp`:

```cpp
GLFWwindow* window = glfwCreateWindow(
    Config::WINDOW_WIDTH,
    Config::WINDOW_HEIGHT,
    "Sandbox de Particulas",
    NULL,
    NULL
);
```

Valores:

```text
1280x720
```

Si luego se baja o sube resolucion por rendimiento, se cambia solo en `config.hpp`.

## RenderParticle

El VBO recibe:

```cpp
struct RenderParticle {
    float x;
    float y;
    float radius;
    int type;
    float energy;
};
```

## Configuracion del VBO

```cpp
glGenVertexArrays(1, &VAO);
glGenBuffers(1, &particleVBO);

glBindVertexArray(VAO);
glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
glBufferData(
    GL_ARRAY_BUFFER,
    sizeof(RenderParticle) * Config::PARTICLE_COUNT,
    nullptr,
    GL_DYNAMIC_DRAW
);

glVertexAttribPointer(
    0, 2, GL_FLOAT, GL_FALSE,
    sizeof(RenderParticle),
    (void*)offsetof(RenderParticle, x)
);
glEnableVertexAttribArray(0);

glVertexAttribPointer(
    1, 1, GL_FLOAT, GL_FALSE,
    sizeof(RenderParticle),
    (void*)offsetof(RenderParticle, radius)
);
glEnableVertexAttribArray(1);

glVertexAttribIPointer(
    2, 1, GL_INT,
    sizeof(RenderParticle),
    (void*)offsetof(RenderParticle, type)
);
glEnableVertexAttribArray(2);

glVertexAttribPointer(
    3, 1, GL_FLOAT, GL_FALSE,
    sizeof(RenderParticle),
    (void*)offsetof(RenderParticle, energy)
);
glEnableVertexAttribArray(3);

glBindVertexArray(0);
```

## Actualizacion del VBO

CPU:

```text
Particle en CPU -> RenderParticle en CPU -> glBufferSubData
```

GPU:

```text
Particle en GPU -> RenderParticle en GPU -> clEnqueueReadBuffer -> glBufferSubData
```

Codigo comun:

```cpp
glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
glBufferSubData(
    GL_ARRAY_BUFFER,
    0,
    sizeof(RenderParticle) * Config::PARTICLE_COUNT,
    renderParticles.data()
);
```

## Vertex shader

`shaders/particles.vert`:

```glsl
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in float aRadius;
layout (location = 2) in int aType;
layout (location = 3) in float aEnergy;

out flat int vType;
out float vEnergy;

uniform float uPointScale;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    gl_PointSize = max(2.0, aRadius * uPointScale);
    vType = aType;
    vEnergy = aEnergy;
}
```

## Fragment shader

`shaders/particles.frag`:

```glsl
#version 330 core

in flat int vType;
in float vEnergy;

out vec4 FragColor;

vec3 materialColor(int type)
{
    if (type == 0) return vec3(1.0, 0.15, 0.10);
    if (type == 1) return vec3(0.15, 0.9, 0.25);
    return vec3(0.2, 0.45, 1.0);
}

void main()
{
    vec2 center = gl_PointCoord - vec2(0.5);
    float d2 = dot(center, center);
    if (d2 > 0.25) discard;

    float alpha = smoothstep(0.25, 0.18, d2);
    vec3 color = materialColor(vType);
    color += clamp(vEnergy, 0.0, 1.0) * 0.20;

    FragColor = vec4(color, alpha);
}
```

## Render

```cpp
glClearColor(0.05f, 0.06f, 0.08f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT);

glEnable(GL_PROGRAM_POINT_SIZE);
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

glUseProgram(shaderProgram);
glUniform1f(
    glGetUniformLocation(shaderProgram, "uPointScale"),
    static_cast<float>(Config::WINDOW_HEIGHT)
);

glBindVertexArray(VAO);
glDrawArrays(GL_POINTS, 0, Config::PARTICLE_COUNT);
```

## Nota fuera de scope

Instanced rendering e interop OpenCL/OpenGL pueden mejorar el rendimiento, pero no son parte del nucleo del proyecto.

