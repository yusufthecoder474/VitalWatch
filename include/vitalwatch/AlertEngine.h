#pragma once
#include "Types.h"
namespace vitalwatch {
class AlertEngine {
    Thresholds thresholds_;
public:
    explicit AlertEngine(Thresholds thresholds = {});
    std::vector<Event> evaluate(const Reading& reading) const;
};
Thresholds loadThresholds(const std::string& path);
}
