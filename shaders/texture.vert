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

layout(location = 0) out vec3 modelNormal; // untransformed normal in model space (for cubemap lookup)
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec4 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPosition, 1.0);
    modelNormal = inNormal;
    fragNormal = normalize(vec3(ubo.view * pc.model * vec4(inNormal, 0.0))); // this will only apply the rotation of the modelview matrix to the normal
    fragTexCoord = inTexCoord;
    fragColor = inColor;
}