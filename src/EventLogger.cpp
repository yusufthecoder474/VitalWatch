#include "vitalwatch/EventLogger.h"
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace vitalwatch {
std::string timestampText(std::int64_t timestamp) {
    const auto seconds = static_cast<std::time_t>(timestamp);
    std::tm utc{};
#ifdef _WIN32
    if (gmtime_s(&utc, &seconds) != 0) throw std::runtime_error("Timestamp conversion failed");
#else
    if (!gmtime_r(&seconds, &utc)) throw std::runtime_error("Timestamp conversion failed");
#endif
    std::ostringstream out;
    out << std::put_time(&utc,"%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}
namespace {
std::string csv(const std::string& value) {
    std::string out = "\"";
    for (char c : value) { if (c == '"') out += '"'; out += c; }
    return out + '"';
}
std::string number(const std::optional<double>& value) {
    if (!value) return "";
    std::ostringstream out; out << std::fixed << std::setprecision(1) << *value; return out.str();
}
}
EventLogger::EventLogger(const std::string& directory) {
    std::filesystem::create_directories(directory);
    readings_.exceptions(std::ios::failbit | std::ios::badbit);
    events_.exceptions(std::ios::failbit | std::ios::badbit);
    // One run per directory: intentionally truncate. Use a new directory to retain older runs.
    readings_.open(std::filesystem::path(directory) / "readings.csv");
    events_.open(std::filesystem::path(directory) / "events.csv");
    readings_ << "timestamp,heart_rate_bpm,spo2_percent,temperature_c,connected\n";
    events_ << "timestamp,type,sensor,message\n";
}
void EventLogger::write(const Reading& r, const std::vector<Event>& events) {
    auto time = timestampText(r.timestamp);
    readings_ << time << ',' << number(r.heartRate) << ',' << number(r.oxygen) << ','
              << number(r.temperature) << ',' << (r.connected ? 1 : 0) << '\n';
    for (const auto& e : events)
        events_ << time << ',' << csv(e.type) << ',' << csv(e.sensor) << ',' << csv(e.message) << '\n';
    readings_.flush(); events_.flush();
}
}
