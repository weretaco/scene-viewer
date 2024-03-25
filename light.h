#ifndef _LIGHT_H
#define _LIGHT_H

#include <iostream>
#include <optional>
#include <string>
#include <variant>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include <glm/gtx/io.hpp>

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

    void print() {
        std::cout << std::endl << "Light Name: " << name << std::endl;
        std::cout << "Tint: " << tint << std::endl;
        std::cout << "Shadow: " << shadow << std::endl;

        if (type == Type::SUN) {
            std::cout << "Angle: " << std::get<SunLight>(value).angle << std::endl;
            std::cout << "Strength: " << std::get<SunLight>(value).strength << std::endl;
        } else if (type == Type::SPHERE) {
            std::cout << "Radius: " << std::get<SphereLight>(value).radius << std::endl;
            std::cout << "Power: " << std::get<SphereLight>(value).power << std::endl;

            std::optional<float> limit = std::get<SphereLight>(value).limit;
            std::cout << "Limit: " << (limit.has_value() ? std::to_string(limit.value()) : "None") << std::endl;
        } else if (type == Type::SPOT) {
            std::cout << "Radius: " << std::get<SpotLight>(value).radius << std::endl;
            std::cout << "Power: " << std::get<SpotLight>(value).power << std::endl;

            std::optional<float> limit = std::get<SpotLight>(value).limit;
            std::cout << "Limit: " << (limit.has_value() ? std::to_string(limit.value()) : "None") << std::endl;

            std::cout << "FOV: " << std::get<SpotLight>(value).fov << std::endl;
            std::cout << "Blend: " << std::get<SpotLight>(value).blend << std::endl;
        }
    }
};

#endif // _LIGHT_H