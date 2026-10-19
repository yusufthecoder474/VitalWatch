#include "vitalwatch/AlertEngine.h"
#include "vitalwatch/EventLogger.h"
#include "vitalwatch/Pipeline.h"
#include "vitalwatch/ReadingValidator.h"
#include "vitalwatch/Replay.h"
#include "vitalwatch/SensorSimulator.h"
#include <atomic>
#include <csignal>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
namespace {
std::atomic<bool> stopped{false};
static_assert(std::atomic<bool>::is_always_lock_free, "Signal flag must be lock-free");
void stop(int) { stopped.store(true, std::memory_order_relaxed); }
unsigned integer(const std::string& s, unsigned maximum) {
    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("Expected unsigned integer: " + s);
    auto value = std::stoull(s);
    if (value > maximum) throw std::invalid_argument("Integer exceeds limit: " + s);
    return static_cast<unsigned>(value);
}
std::string value(const std::optional<double>& v) {
    if (!v) return "--";
    std::ostringstream out; out << std::fixed << std::setprecision(1) << *v; return out.str();
}
}
int main(int argc, char** argv) {
    using namespace vitalwatch;
    try {
        unsigned count = 16, interval = 250, seed = 42;
        bool deterministic = false;
        std::string replay, config, logs = "logs";
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--help") {
                std::cout << "VitalWatch --deterministic --count N --interval-ms N --seed N\n"
                             "           --replay FILE --config FILE --log-dir DIR\n"
                             "Count: 1..10000, interval: 0..1000 ms. Replay uses all rows.\n";
                return 0;
            }
            if (arg == "--deterministic") { deterministic = true; continue; }
            if (arg != "--count" && arg != "--interval-ms" && arg != "--seed" &&
                arg != "--replay" && arg != "--config" && arg != "--log-dir")
                throw std::invalid_argument("Unknown option: " + arg);
            if (++i >= argc) throw std::invalid_argument("Missing value for " + arg);
            std::string v = argv[i];
            if (arg == "--count") { count = integer(v,10000); if (!count) throw std::invalid_argument("Count must be positive"); }
            else if (arg == "--interval-ms") interval = integer(v,1000);
            else if (arg == "--seed") seed = integer(v,1000000);
            else if (arg == "--replay") replay = v;
            else if (arg == "--config") config = v;
            else logs = v;
        }
        Thresholds thresholds = config.empty() ? Thresholds{} : loadThresholds(config);
        AlertEngine alerts(thresholds);
        ReadingValidator validator;
        std::vector<Reading> replayRows;
        if (!replay.empty()) replayRows = loadReplay(replay);
        EventLogger logger(logs);
        SensorSimulator simulator(deterministic,seed,deterministic ? 1767225600LL : static_cast<std::int64_t>(std::time(nullptr)));
        std::signal(SIGINT,stop);
        std::cout << "VitalWatch | EDUCATIONAL SIMULATOR | NON-CLINICAL DEMO THRESHOLDS\n"
                  << "HR: " << thresholds.hrMin << ".." << thresholds.hrMax
                  << "  SpO2: " << thresholds.oxygenMin << ".." << thresholds.oxygenMax
                  << "  Temp C: " << thresholds.tempMin << ".." << thresholds.tempMax << '\n'
                  << "UTC timestamp         HR BPM   SpO2 %   Temp C   Sensor/event status\n";
        std::size_t index = 0, processed = 0;
        runPipeline([&]() -> std::optional<Reading> {
            if (stopped.load(std::memory_order_relaxed)) return std::nullopt;
            if (!replay.empty()) {
                if (index == replayRows.size()) return std::nullopt;
                return replayRows[index++];
            }
            if (index++ == count) return std::nullopt;
            return simulator.next();
        }, [&](const Reading& r) {
            auto events = validator.validate(r);
            auto violations = alerts.evaluate(r);
            events.insert(events.end(),violations.begin(),violations.end());
            logger.write(r,events);
            std::cout << timestampText(r.timestamp) << "  " << std::setw(7) << value(r.heartRate)
                      << "  " << std::setw(7) << value(r.oxygen) << "  " << std::setw(7) << value(r.temperature)
                      << "  " << (r.connected ? "CONNECTED" : "DISCONNECTED");
            for (const auto& e : events) std::cout << " | " << e.type << ':' << e.sensor;
            std::cout << '\n'; ++processed;
        },interval);
        std::cout << "Processed " << processed << " readings. Logs: " << logs << "\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "Error: " << e.what() << '\n'; return 1; }
}
