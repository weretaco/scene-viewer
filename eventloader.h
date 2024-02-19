#ifndef _EVENT_LOADER_H
#define _EVENT_LOADER_H

#include <fstream>
#include <vector>

class EventLoader {
public:
    EventLoader(std::string filename);

    void parseEvents();

    void close();

private:

    std::fstream fileStream;
};

#endif // _JSON_LOADER_H