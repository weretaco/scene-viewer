#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant, std430) uniform PushConstant {
    mat4 model;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec4 inColor;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 camPos;

void main() {
    fragNormal = mat3(transpose(inverse(pc.model))) * inNormal;
    fragPos = vec3(pc.model * vec4(inPosition, 1.0));
    camPos = vec3(inverse(ubo.view) * vec4(0.0, 0.0, 0.0, 1.0));

    gl_Position = ubo.proj * ubo.view * vec4(fragPos, 1.0);
}