#ifndef _LIGHT_H
#define _LIGHT_H

#include <optional>
#include <string>
#include <variant>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"

struct SunLight {
    float angle;
    float strength;
};

struct SphereLight {
    float radius;
    float power;
    std::optional<float> limit;
};

struct SpotLight {
    float radius;
    float power;
    std::optional<float> limit;
    float fov;
    float blend;
};

struct Light {
    std::string name;
    glm::vec3 tint; // default to <1, 1, 1>
    uint16_t shadow; // default to 0

    enum class Type {
        SUN,
        SPHERE,
        SPOT
    };
    std::variant<
        SunLight,
        SphereLight,
        SpotLight,
        std::monostate
    > value;
    Type type;
};

#endif // _LIGHT_H