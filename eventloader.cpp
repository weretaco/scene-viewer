#include "eventloader.h"

#include <iostream>
#include <string>

EventLoader::EventLoader(std::string fileName) {
    fileStream.open(fileName, std::ios::in);

    if (fileStream.fail()) {
        throw std::runtime_error("Failed to open events file: " + fileName);
    }
}

void EventLoader::parseEvents() {
    std::cout << "PARTY TIME" << std::endl;

    std::string line;
    while (std::getline(fileStream, line)) {
        std::cout << line << std::endl;
    }
}

void EventLoader::close() {
    fileStream.close();
}