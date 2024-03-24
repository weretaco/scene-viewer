#ifndef _SCENE_H
#define _SCENE_H

#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

#include "animation.h"
#include "camera.h"
#include "driver.h"
#include "environment.h"
#include "light.h"
#include "material.h"
#include "mesh.h"
#include "node.h"

struct Scene {
    std::vector<Node> nodes;
    std::vector<Mesh> meshes;
    std::vector<Camera> cameras;
    std::vector<Driver> drivers;
    std::vector<Animation> anims;
    std::vector<Material> materials;
    std::vector<Environment> environments;
    std::vector<Light> lights;
    std::vector<uint16_t> roots;

    std::vector<VulkanSampledImage> vulkanImages;

    // maps indices of JSON nodes to the index of the corresponding struct in one of the arrays of the Scene object
    // EXAMPLE: If the first mesh is at index 5 in the JSON array, then typeIndices[5] == 0.
    //          Similary, if the first node is at index 3, then typeIndices[3] == 0 as well
    std::vector<uint16_t> typeIndices;

    void print() {
        std::cout << "Scene:" << std::endl;
        
        std::cout << std::endl << "ROOTS:" << std::endl;
        for (uint16_t& root : roots) {
            std::cout << root << " ";
        }
        std::cout << std::endl;

        std::cout << std::endl << "CAMERAS:" << std::endl;
        for (Camera& camera : cameras) {
            camera.print();
        }
        std::cout << std::endl;

        std::cout << std::endl << "NODES:" << std::endl;
        for (Node& node : nodes) {
            node.print();
        }
        std::cout << std::endl;

        std::cout << std::endl << "MESHES:" << std::endl;
        for (Mesh& mesh : meshes) {
            mesh.print();
        }
        std::cout << std::endl;

        std::cout << std::endl << "DRIVERS:" << std::endl;
        for (Driver& driver : drivers) {
            driver.print();
        }
        std::cout << std::endl;
    }

    void cleanup(VkDevice device) {
        for (Material& mat : materials) {
            mat.cleanup(device);
        }
    }
};

#endif // _SCENE_H