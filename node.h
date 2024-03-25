#ifndef _NODE_H
#define _NODE_H

#include <iostream>
#include <optional>
#include <vector>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include <glm/gtx/io.hpp>
#include "glm/gtx/quaternion.hpp"

struct Node {
    std::string name;
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    std::vector<uint16_t> children;
    std::optional<uint16_t> camera;
    std::optional<uint16_t> mesh;
    std::optional<uint16_t> environment;
    std::optional<uint16_t> light;

    void print() {
        std::cout << "Name: " << name << std::endl;

        std::cout << "Translation: " << translation << std::endl;
        std::cout << "Rotation: " << rotation << std::endl;
        std::cout << "Scale: " << scale << std::endl;

        std::cout << "Children: ";
        for (uint16_t child : children) {
            std:: cout << child << " ";
        }
        std::cout << std::endl;

        if (camera.has_value()) {
            std::cout << "Camera: " << camera.value() << std::endl;
        }
        
        if (mesh.has_value()) {
            std::cout << "Mesh: " << mesh.value() << std::endl;
        }
    }
};

#endif // _NODE_H