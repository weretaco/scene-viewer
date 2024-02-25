#ifndef _ATTRIBUTE_H
#define _ATTRIBUTE_H

#include <string>
#include <iostream>

struct Attribute {
    std::string name;
    std::string src;
    uint32_t offset;
    uint32_t stride;
    std::string format;

    void print() {
        std::cout << "Attribute Name: " << name << std::endl;
        std::cout << "    Src: " << src << std::endl;
        std::cout << "    Offset: " << offset << std::endl;
        std::cout << "    Stride: " << stride << std::endl;
        std::cout << "    Format: " << format << std::endl;
    }
};

#endif // _ATTRIBUTE_H