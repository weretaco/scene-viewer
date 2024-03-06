#version 450

layout(binding = 1) uniform samplerCube texSampler;

layout(location = 0) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texSampler, normalize(fragNormal));

    outColor = vec4(texColor.rgb, 1.0);
}