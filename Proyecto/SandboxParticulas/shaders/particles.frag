#version 330 core

flat in int vType;
in float vEnergy;

out vec4 FragColor;

vec3 materialColor(int type)
{
    if (type == 0) {
        return vec3(1.0, 0.15, 0.10);
    }
    if (type == 1) {
        return vec3(0.15, 0.90, 0.25);
    }
    return vec3(0.20, 0.45, 1.0);
}

void main()
{
    vec2 center = gl_PointCoord - vec2(0.5);
    float d2 = dot(center, center);
    if (d2 > 0.25) {
        discard;
    }

    float alpha = smoothstep(0.25, 0.18, d2);
    vec3 color = materialColor(vType);
    color += clamp(vEnergy, 0.0, 1.0) * 0.20;

    FragColor = vec4(color, alpha);
}
