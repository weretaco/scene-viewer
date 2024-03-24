#ifndef _DRIVER_H
#define _DRIVER_H

#include <iostream>
#include <string>
#include <vector>

struct Driver {
    std::string name;
    uint16_t node;
    std::string channel;
    std::vector<float> times;
    std::vector<float> values;
    std::string interpolation;
    uint16_t animIndex;

    void print() {
        std::cout << "Name: " << name << std::endl;

        std::cout << "Node: " << node << std::endl;
        std::cout << "Channel: " << channel << std::endl;
        std::cout << "Interpolation: " << interpolation << std::endl;

        std::cout << "Times: ";
        for (float time : times) {
            std::cout << time << " ";
        }
        std::cout << std::endl;

        std::cout << "Values: ";
        for (float value : values) {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
};

#endif // _DRIVER_H