#include <cstdlib>
#include <iostream>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

struct Attribute {
    std::string src;
    uint32_t offset;
    uint32_t stride;
    std::string format;
};

void loadScene() {
    std::string topology = "TRIANGLE_LIST";
    int count = 468; // number of vertices

    Attribute position {
        .src = "scenes/sg-Support.Cylinder.b72",
        .offset = 0,
        .stride = 28,
        .format = "R32G32B32_SFLOAT"
    };

    Attribute normal {
        .src = "scenes/sg-Support.Cylinder.b72",
        .offset = 12,
        .stride = 28,
        .format = "R32G32B32_SFLOAT"
    };

    Attribute color {
        .src = "scenes/sg-Support.Cylinder.b72",
        .offset = 24,
        .stride = 28,
        .format = "R8G8B8A8_UNORM"
    };

    std::ifstream vertexData;
    vertexData.open(position.src, std::ios::binary | std::ios::in);

    glm::vec3 pos;
    glm::vec3 norm;
    uint32_t col;

    std::cout << "SIZES: " << std::endl << std::endl;

    std::cout << "vec3: " << sizeof(glm::vec3) << std::endl;
    std::cout << "bvec4: " << sizeof(glm::bvec4) << std::endl;

    std::cout << "DATA: " << std::endl << std::endl;

    for (uint32_t i = 0; i < count; i++) {
        vertexData.read(reinterpret_cast<char*>(&pos), sizeof(glm::vec3));
        vertexData.read(reinterpret_cast<char*>(&norm), sizeof(glm::vec3));
        vertexData.read(reinterpret_cast<char*>(&col), sizeof(uint32_t));
        
        std::cout << "pos: " << glm::to_string(pos) << std::endl;
        std::cout << "norm: " << glm::to_string(norm) << std::endl;
        std::cout << "col: 0x"  << std::hex << col  << std::dec << std::endl;
        std::cout << std::endl;
    }

    vertexData.close();
}

int main() {
    loadScene();

    std::cout << "Finished!" << std::endl;

    std::cin.get();

    return EXIT_SUCCESS;
}