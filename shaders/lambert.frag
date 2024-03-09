#version 450

layout(binding = 1) uniform sampler2D albedoMap;
layout(binding = 2) uniform sampler2D normalMap;
layout(binding = 3) uniform samplerCube irradianceMap;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 albedo = texture(albedoMap, fragTexCoord).rgb;
    vec3 irradiance = texture(irradianceMap, normalize(fragNormal)).rgb;

    outColor = vec4(albedo * irradiance, 1.0);
}