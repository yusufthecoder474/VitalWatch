#pragma once
#include "Types.h"
#include <functional>
namespace vitalwatch {
// Each callback is owned by exactly one worker. Exceptions are rethrown after joins.
void runPipeline(const std::function<std::optional<Reading>()>& source,
                 const std::function<void(const Reading&)>& sink, unsigned intervalMs = 0);
}
