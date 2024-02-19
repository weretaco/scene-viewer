#ifndef _EVENT_LOADER_H
#define _EVENT_LOADER_H

#include <fstream>
#include <sstream>
#include <vector>

class EventLoader {
public:

    struct Event {
        enum class Type {
            AVAILABLE,
            PLAY,
            SAVE,
            MARK
        };
        uint32_t timestamp;
        Type type;
        std::vector<std::string> args;
    };

    EventLoader(std::string filename);

    std::vector<EventLoader::Event> parseEvents();

    void close();

private:

    std::fstream fileStream;

    std::vector<std::string> splitLine(const std::string& line);
};

#endif // _JSON_LOADER_H