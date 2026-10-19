#include "vitalwatch/AlertEngine.h"
#include "vitalwatch/ReadingValidator.h"
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
namespace vitalwatch {
AlertEngine::AlertEngine(Thresholds t) : thresholds_(t) {
    auto check = [](double low, double high, double domainLow, double domainHigh) {
        return ReadingValidator::validValue(low, domainLow, domainHigh) &&
               ReadingValidator::validValue(high, domainLow, domainHigh) && low <= high;
    };
    if (!check(t.hrMin,t.hrMax,0,300) || !check(t.oxygenMin,t.oxygenMax,0,100) ||
        !check(t.tempMin,t.tempMax,20,50)) throw std::invalid_argument("Invalid demonstration thresholds");
}
std::vector<Event> AlertEngine::evaluate(const Reading& r) const {
    std::vector<Event> events;
    if (!r.connected) return events;
    auto check = [&](const std::optional<double>& v, const char* sensor, double domainLow,
                     double domainHigh, double low, double high) {
        if (v && ReadingValidator::validValue(*v, domainLow, domainHigh) && (*v < low || *v > high))
            events.push_back({"DEMO_ALERT", sensor, "Outside non-clinical demonstration thresholds"});
    };
    check(r.heartRate,"heart_rate",0,300,thresholds_.hrMin,thresholds_.hrMax);
    check(r.oxygen,"oxygen",0,100,thresholds_.oxygenMin,thresholds_.oxygenMax);
    check(r.temperature,"temperature",20,50,thresholds_.tempMin,thresholds_.tempMax);
    return events;
}
Thresholds loadThresholds(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open config: " + path);
    Thresholds t;
    std::map<std::string,double*> fields{{"hr_min",&t.hrMin},{"hr_max",&t.hrMax},
        {"oxygen_min",&t.oxygenMin},{"oxygen_max",&t.oxygenMax},{"temp_min",&t.tempMin},{"temp_max",&t.tempMax}};
    std::set<std::string> seen;
    std::string line;
    while (std::getline(in,line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        auto pos = line.find('=');
        if (pos == std::string::npos) throw std::runtime_error("Malformed config line");
        auto key = line.substr(0,pos);
        auto value = line.substr(pos+1);
        if (!fields.count(key) || !seen.insert(key).second) throw std::runtime_error("Unknown or duplicate config key: " + key);
        std::size_t used = 0;
        double number = std::stod(value,&used);
        if (used != value.size()) throw std::runtime_error("Malformed config number");
        *fields.at(key) = number;
    }
    if (in.bad()) throw std::runtime_error("Config read failed");
    (void)AlertEngine(t);
    return t;
}
}
