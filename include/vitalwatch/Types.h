#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace vitalwatch {
struct Reading {
    std::int64_t timestamp = 0; // Unix seconds, UTC
    std::optional<double> heartRate, oxygen, temperature;
    bool connected = true; // Connection of the simulated three-channel sensor unit
};
struct Thresholds {
    double hrMin = 60, hrMax = 100;
    double oxygenMin = 90, oxygenMax = 100;
    double tempMin = 36, tempMax = 38;
};
struct Event { std::string type, sensor, message; };
std::string timestampText(std::int64_t timestamp);
}
