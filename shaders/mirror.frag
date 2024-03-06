#version 450

layout(binding = 1) uniform samplerCube texSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 camPos;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 I = normalize(fragPos - camPos);
    vec3 R = reflect(I, normalize(fragNormal));
    vec4 texColor = texture(texSampler, R);

    outColor = vec4(texColor.rgb, 1.0);
}