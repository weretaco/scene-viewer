#ifndef _CAMERA_H
#define _CAMERA_H

#include <iostream>
#include <string>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"

// assume perspective camera
struct Camera {
    std::string name;
    float vfov;
    float aspect;
    float near;
    float far;
    glm::mat4 viewMat;

    void print() {
        std::cout << "Name: " << name << std::endl;
        std::cout << "Aspect: " << aspect << std::endl;
        std::cout << "Vfov: " << vfov << std::endl;
        std::cout << "Near: " << near << std::endl;
        std::cout << "Far: " << far << std::endl;
        std::cout << "View Mat: " << viewMat << std::endl;
    }
};

#endif // _CAMERA_H