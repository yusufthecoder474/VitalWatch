#include "vitalwatch/ReadingValidator.h"
#include <cmath>
namespace vitalwatch {
bool ReadingValidator::validValue(double value, double minimum, double maximum) {
    return std::isfinite(value) && value >= minimum && value <= maximum;
}
std::vector<Event> ReadingValidator::validate(const Reading& r) const {
    if (!r.connected) return {{"DISCONNECTED", "unit", "Simulated sensor unit disconnected; values ignored"}};
    std::vector<Event> events;
    auto check = [&](const std::optional<double>& v, const char* sensor, double low, double high) {
        if (!v) events.push_back({"MISSING", sensor, "No value in this sample"});
        else if (!validValue(*v, low, high)) events.push_back({"INVALID", sensor, "Outside simulator input domain"});
    };
    check(r.heartRate, "heart_rate", 0, 300);
    check(r.oxygen, "oxygen", 0, 100);
    check(r.temperature, "temperature", 20, 50);
    return events;
}
}
