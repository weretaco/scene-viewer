#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

#include <string>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"

#include "material.h"

struct Environment {
    std::string name;
    Texture<glm::vec3> radiance;
};

#endif // _ENVIRONMENT_H