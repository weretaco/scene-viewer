#ifndef _ANIMATION_H
#define _ANIMATION_H

#include <chrono>

struct Animation {
    std::chrono::high_resolution_clock::time_point startTime;
    uint16_t curFrameIndex;
};

#endif // _ANIMATION_H