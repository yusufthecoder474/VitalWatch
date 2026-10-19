#pragma once
#include "Types.h"
namespace vitalwatch {
class ReadingValidator {
public:
    static bool validValue(double value, double minimum, double maximum);
    std::vector<Event> validate(const Reading& reading) const;
};
}
