#include "vitalwatch/Replay.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
namespace vitalwatch {
std::vector<Reading> loadReplay(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open replay: " + path);
    std::string line;
    auto readLine = [&]() {
        bool ok = static_cast<bool>(std::getline(in,line));
        if (!line.empty() && line.back() == '\r') line.pop_back();
        return ok;
    };
    if (!readLine() || line != "timestamp,heart_rate,oxygen,temperature,connected")
        throw std::runtime_error("Replay header must match sample exactly");
    std::vector<Reading> rows;
    std::size_t lineNo = 1;
    while (readLine()) {
        ++lineNo;
        try {
            std::vector<std::string> fields;
            std::istringstream row(line);
            std::string field;
            while (std::getline(row,field,',')) fields.push_back(field);
            if (!line.empty() && line.back() == ',') fields.emplace_back();
            if (fields.size() != 5) throw std::runtime_error("Expected five fields");
            std::size_t used = 0;
            auto time = std::stoll(fields[0],&used);
            if (used != fields[0].size() || time < 0 || time > 4102444800LL)
                throw std::runtime_error("Timestamp must be Unix seconds from 1970 through 2100");
            if (!rows.empty() && time <= rows.back().timestamp) throw std::runtime_error("Timestamps must increase");
            auto parse = [](const std::string& s)->std::optional<double> {
                if (s.empty()) return std::nullopt;
                std::size_t n = 0; auto v = std::stod(s,&n);
                if (n != s.size() || !std::isfinite(v)) throw std::runtime_error("Malformed finite number");
                return v;
            };
            if (fields[4] != "0" && fields[4] != "1") throw std::runtime_error("Connected must be 0 or 1");
            rows.push_back({time,parse(fields[1]),parse(fields[2]),parse(fields[3]),fields[4] == "1"});
        } catch (const std::exception& e) {
            throw std::runtime_error("Replay line " + std::to_string(lineNo) + ": " + e.what());
        }
    }
    if (in.bad()) throw std::runtime_error("Replay read failed");
    if (rows.empty()) throw std::runtime_error("Replay contains no samples");
    return rows;
}
}
