#include "vitalwatch/SensorSimulator.h"
namespace vitalwatch {
SensorSimulator::SensorSimulator(bool deterministic, unsigned seed, std::int64_t start)
    : random_(seed), start_(start), deterministic_(deterministic) {}
Reading SensorSimulator::next() {
    Reading r;
    r.timestamp = start_ + static_cast<std::int64_t>(index_);
    if (deterministic_) {
        r.heartRate = 75; r.oxygen = 97; r.temperature = 37;
    } else {
        r.heartRate = 70 + random_() % 16;
        r.oxygen = 95 + random_() % 5;
        r.temperature = 36.5 + static_cast<double>(random_() % 10) / 10;
    }
    // A repeatable eight-row demo covers each fault category.
    switch (index_ % 8) {
        case 1: r.heartRate = 120; break;
        case 2: r.oxygen = 85; break;
        case 3: r.temperature = 39; break;
        case 4: r.heartRate = -5; break;
        case 5: r.oxygen.reset(); break;
        case 6: r.connected = false; r.heartRate.reset(); r.oxygen.reset(); r.temperature.reset(); break;
        default: break;
    }
    ++index_;
    return r;
}
}
