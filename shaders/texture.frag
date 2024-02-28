#version 450

//layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 1) uniform samplerCube texSampler;

layout(location = 0) in vec3 modelNormal;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    //vec4 texColor = texture(texSampler, fragTexCoord);
    vec4 texColor = texture(texSampler, modelNormal);

    vec3 light = mix(vec3(0,0,0), vec3(1,1,1), dot(fragNormal, vec3(0,0,1)) * 0.5 + 0.5);

    //outColor = vec4(light * fragColor.rgb, fragColor.a);
    outColor = vec4(light * texColor.rgb, texColor.a);
}