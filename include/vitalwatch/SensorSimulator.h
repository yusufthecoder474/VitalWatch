#pragma once
#include "Types.h"
#include <cstddef>
#include <random>
namespace vitalwatch {
class SensorSimulator {
    std::mt19937 random_;
    std::size_t index_ = 0;
    std::int64_t start_;
    bool deterministic_;
public:
    SensorSimulator(bool deterministic, unsigned seed, std::int64_t start);
    Reading next();
};
}
