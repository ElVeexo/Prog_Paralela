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

