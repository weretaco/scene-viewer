#include "eventloader.h"

#include <iostream>
#include <string>

EventLoader::EventLoader(std::string fileName) {
    fileStream.open(fileName, std::ios::in);

    if (fileStream.fail()) {
        throw std::runtime_error("Failed to open events file: " + fileName);
    }
}

std::vector<EventLoader::Event> EventLoader::parseEvents() {
    std::vector<EventLoader::Event> events;

    std::string line;
    while (std::getline(fileStream, line)) {
        std::vector<std::string> params = splitLine(line);

        events.push_back({});

        events.back().timestamp = stol(params[0]);

        if (params[1] == "AVAILABLE") {
            events.back().type = Event::Type::AVAILABLE;
        } else if (params[1] == "PLAY") {
            events.back().type = Event::Type::PLAY;
        } else if (params[1] == "SAVE") {
            events.back().type = Event::Type::SAVE;
        } else if (params[1] == "MARK") {
            events.back().type = Event::Type::MARK;
        } else {
            throw std::runtime_error("Unexpected event type: " + params[1]);
        }

        for (int i = 2; i < params.size(); i++) {
            events.back().args.push_back(params[i]);
        }
    }

    return events;
}

void EventLoader::close() {
    fileStream.close();
}

std::vector<std::string> EventLoader::splitLine(const std::string& line) {
    std::vector<std::string> params;

    std::string param;
    std::stringstream ss{line};

    while (std::getline(ss, param, ' ')) {
        params.push_back(param);
    }

    return params;
}