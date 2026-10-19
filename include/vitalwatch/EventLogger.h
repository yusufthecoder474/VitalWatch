#pragma once
#include "Types.h"
#include <fstream>
namespace vitalwatch {
class EventLogger {
    std::ofstream readings_, events_;
public:
    explicit EventLogger(const std::string& directory);
    void write(const Reading& reading, const std::vector<Event>& events);
};
}
