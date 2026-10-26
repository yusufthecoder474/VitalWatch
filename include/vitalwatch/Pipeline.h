#pragma once
#include "Types.h"
#include <cstddef>
#include <functional>
namespace vitalwatch {
// Each callback is owned by exactly one worker. Exceptions are rethrown after joins.
void runPipeline(const std::function<std::optional<Reading>()>& source,
                 const std::function<void(const Reading&)>& sink, unsigned intervalMs = 0,
                 std::size_t queueCapacity = 0);
}
