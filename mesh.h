#ifndef _MESH_H
#define _MESH_H

#include <array>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <vulkan/vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"

#include "attribute.h"
#include "vertexcolor.h"
#include "vertextexture.h"
#include "vertexlambert.h"
#include "vertexpbr.h"

struct Indices {
    std::string src;
    uint32_t offset;
    std::string format;
};

struct Mesh {
    std::string name;
    std::string topology;
    uint32_t vertexCount;
    std::string src;
    uint32_t stride;
    Indices indicesData;
    std::vector<Attribute> attributes;
    std::optional<uint16_t> material;

    std::variant<
        std::vector<VertexColor>,
        std::vector<VertexTexture>,
        std::vector<VertexLambert>,
        std::vector<VertexPBR>
    > vertices;

    std::vector<uint16_t> indices;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    std::array<glm::vec3, 2> aabb; // define min point and max point for AABB

    void cleanup(VkDevice device) {
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
    }

    void print() {
        std::cout << "Name: " << name << std::endl;
        std::cout << "Topology: " << topology << std::endl;
        std::cout << "Vertex Count: " << vertexCount << std::endl;
        std::cout << "Src: " << src << std::endl;
        std::cout << "Stride: " << stride << std::endl;

        if (indicesData.src != "") {
            std::cout << "indices.Src: " << indicesData.src << std::endl;
            std::cout << "indices.Offset: " << indicesData.offset << std::endl;
            std::cout << "indices.Format: " << indicesData.format << std::endl;
        }

        for (Attribute& attr : attributes) {
            attr.print();
        }
    }
};

#endif // _MESH_H