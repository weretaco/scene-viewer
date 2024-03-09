#version 450

layout(binding = 1) uniform sampler2D albedoMap;
layout(binding = 2) uniform sampler2D normalMap;
layout(binding = 3) uniform sampler2D metallicMap;
layout(binding = 4) uniform sampler2D roughnessMap;

layout(binding = 5) uniform samplerCube irradianceMap;

// these have something to do with the importance-sampled mipmaps I'm supposed to generate in my cube util
layout(binding = 6) uniform samplerCube prefilterMap;
layout(binding = 7) uniform sampler2D brdfLUT;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 camPos; // should really be a uniform

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

vec3 getNormalFromMap() {
    vec3 tangentNormal = texture(normalMap, fragTexCoord).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(fragPos);
    vec3 Q2  = dFdy(fragPos);
    vec2 st1 = dFdx(fragTexCoord);
    vec2 st2 = dFdy(fragTexCoord);

    vec3 N = normalize(fragNormal);
    vec3 T = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main() {
    vec3 albedo = pow(texture(albedoMap, fragTexCoord).rgb, vec3(2.2));
    float metallic = texture(metallicMap, fragTexCoord).r;
    float roughness = texture(roughnessMap, fragTexCoord).r;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(camPos - fragPos);
    vec3 R = reflect(-V, N);

    /*
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic); ...

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo;
    */

    // reflectance equation
    vec3 Lo = vec3(0.0);

    // TODO: Calculate Lo

    // ambient lighting (note that the next IBL tutorial will replace 
    // this ambient lighting with environment lighting).
    vec3 ambient = vec3(0.03) * albedo;

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    outColor = vec4(color , 1.0);
}