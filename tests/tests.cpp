#include "vitalwatch/AlertEngine.h"
#include "vitalwatch/EventLogger.h"
#include "vitalwatch/Pipeline.h"
#include "vitalwatch/ReadingValidator.h"
#include "vitalwatch/Replay.h"
#include "vitalwatch/SensorSimulator.h"
#include "vitalwatch/ThreadSafeQueue.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace vitalwatch;
namespace {
// Unlike assert(), these checks are always active under NDEBUG/Release.
void require(bool ok, const std::string& message) { if (!ok) throw std::runtime_error(message); }
template<class F> void throws(F action) {
    bool caught = false;
    try { action(); } catch (const std::exception&) { caught = true; }
    require(caught,"Expected exception");
}
Reading normal() { return {1767225600,75,97,37,true}; }
std::string contents(const std::filesystem::path& p) {
    std::ifstream in(p); require(static_cast<bool>(in),"Open test output");
    std::ostringstream out; out << in.rdbuf(); return out.str();
}
struct TempDirectory {
    std::filesystem::path path;
    explicit TempDirectory(const std::string& name) : path("test-output-" + name) {
        std::filesystem::remove_all(path); std::filesystem::create_directories(path);
    }
    ~TempDirectory() { std::error_code ec; std::filesystem::remove_all(path,ec); }
    std::string file(const std::string& name, const std::string& text) {
        auto p = path/name; std::ofstream out(p); out << text; out.close();
        require(static_cast<bool>(out),"Write fixture"); return p.string();
    }
};
void run(const std::string& name) {
    ReadingValidator validator;
    AlertEngine engine;
    auto r = normal();
    TempDirectory temp(name);
    if (name == "valid") {
        require(validator.validate(r).empty() && engine.evaluate(r).empty(),"Normal sample");
    } else if (name == "invalid_hr" || name == "invalid_oxygen" || name == "invalid_temperature") {
        std::string sensor;
        if (name == "invalid_hr") { r.heartRate = -1; sensor = "heart_rate"; }
        if (name == "invalid_oxygen") { r.oxygen = 101; sensor = "oxygen"; }
        if (name == "invalid_temperature") { r.temperature = 10; sensor = "temperature"; }
        auto events = validator.validate(r);
        require(events.size() == 1 && events[0].type == "INVALID" && events[0].sensor == sensor,"Invalid sensor category");
        require(engine.evaluate(r).empty(),"Invalid input must not become threshold alert");
    } else if (name == "threshold") {
        r.heartRate = 120; r.oxygen = 85; r.temperature = 39;
        auto events = engine.evaluate(r);
        require(events.size() == 3,"All three demo violations");
        for (const auto& e : events) require(e.type == "DEMO_ALERT","Alert type");
    } else if (name == "missing") {
        r.oxygen.reset(); auto events = validator.validate(r);
        require(events.size() == 1 && events[0].type == "MISSING" && events[0].sensor == "oxygen","Missing oxygen");
        require(engine.evaluate(r).empty(),"Missing value cannot alert");
    } else if (name == "disconnected") {
        r.connected = false; r.heartRate = 120; r.oxygen.reset();
        auto events = validator.validate(r);
        require(events.size() == 1 && events[0].type == "DISCONNECTED","Disconnect takes precedence");
        require(engine.evaluate(r).empty(),"Disconnected stale values ignored");
    } else if (name == "csv") {
        { EventLogger logger(temp.path.string());
          logger.write(r,{{"DEMO_ALERT","heart_rate","demo, \"quoted\""}}); }
        auto readings = contents(temp.path/"readings.csv");
        auto events = contents(temp.path/"events.csv");
        require(readings == "timestamp,heart_rate_bpm,spo2_percent,temperature_c,connected\n2026-01-01T00:00:00Z,75.0,97.0,37.0,1\n","CSV row/header/time");
        require(events.find("\"demo, \"\"quoted\"\"\"") != std::string::npos,"CSV escaping");
        // Failed output initialization propagates, instead of silently dropping data.
        auto blocker = temp.file("blocker","file");
        throws([&] { EventLogger logger(blocker + "/child"); });
    } else if (name == "shutdown") {
        int produced = 0;
        std::vector<std::int64_t> received;
        runPipeline([&]() -> std::optional<Reading> {
            if (produced == 1000) return std::nullopt;
            auto sample = normal(); sample.timestamp += produced++; return sample;
        },[&](const Reading& sample) { received.push_back(sample.timestamp); });
        require(received.size() == 1000,"Queue drains all samples before shutdown");
        for (int i=0; i<1000; ++i) require(received[static_cast<std::size_t>(i)] == r.timestamp+i,"FIFO exactly once");
        runPipeline([]()->std::optional<Reading> { return std::nullopt; },[](const Reading&) { throw std::runtime_error("Unexpected sample"); });
    } else if (name == "malformed") {
        const std::string header = "timestamp,heart_rate,oxygen,temperature,connected\n";
        for (const auto& row : {"1,abc,97,37,1", "1,75,97,37,2", "1,75,97,37,1,extra", "1,75,97", "-1,75,97,37,1", "1,75,97,37,1junk", "1,nan,97,37,1"}) {
            auto p = temp.file("bad.csv",header + row + "\n");
            throws([&] { (void)loadReplay(p); });
        }
        auto p = temp.file("empty.csv",header); throws([&] { (void)loadReplay(p); });
        p = temp.file("badheader.csv","wrong\n"); throws([&] { (void)loadReplay(p); });
        p = temp.file("time.csv",header+"2,75,97,37,1\n1,75,97,37,1\n");
        throws([&] { (void)loadReplay(p); });
    } else if (name == "deterministic") {
        SensorSimulator a(true,42,r.timestamp), b(true,999,r.timestamp);
        std::size_t faults = 0, alerts = 0;
        for (int i=0; i<8; ++i) {
            auto x=a.next(), y=b.next();
            require(x.timestamp==r.timestamp+i && x.heartRate==y.heartRate && x.oxygen==y.oxygen &&
                    x.temperature==y.temperature && x.connected==y.connected,"Deterministic sequence independent of seed");
            faults += validator.validate(x).size(); alerts += engine.evaluate(x).size();
        }
        require(faults==3 && alerts==3,"Expected fault and alert cycle");
        SensorSimulator c(false,7,r.timestamp),d(false,7,r.timestamp);
        for (int i=0;i<16;++i) { auto x=c.next(),y=d.next();
            require(x.heartRate==y.heartRate && x.oxygen==y.oxygen && x.temperature==y.temperature,"Seed reproducibility"); }
    } else if (name == "boundaries") {
        r.heartRate=60; r.oxygen=90; r.temperature=36;
        require(engine.evaluate(r).empty(),"Inclusive lower demo bounds");
        r.heartRate=100; r.oxygen=100; r.temperature=38;
        require(engine.evaluate(r).empty(),"Inclusive upper demo bounds");
        r.heartRate=0; r.oxygen=0; r.temperature=20;
        require(validator.validate(r).empty(),"Input domain lower bounds");
        r.heartRate=300; r.oxygen=100; r.temperature=50;
        require(validator.validate(r).empty(),"Input domain upper bounds");
    } else if (name == "config") {
        auto p=temp.file("demo.cfg","# demo\nhr_max=80\n");
        AlertEngine custom(loadThresholds(p)); r.heartRate=85;
        require(custom.evaluate(r).size()==1 && engine.evaluate(r).empty(),"Config changes rule");
    } else if (name == "config_invalid") {
        for (const auto& text : {"hr_min=200\nhr_max=100\n", "hr_max=nan\n", "bad=1\n", "hr_max=90\nhr_max=95\n", "hr_max=90abc\n", "hr_max\n", "oxygen_max=101\n"}) {
            auto p=temp.file("bad.cfg",text); throws([&] { (void)loadThresholds(p); });
        }
    } else if (name == "producer_error") {
        int n=0, received=0;
        throws([&] { runPipeline([&]()->std::optional<Reading> {
            if (n++==3) throw std::runtime_error("Injected producer failure");
            return normal();
        },[&](const Reading&) { ++received; }); });
        require(received==3,"Accepted samples drained after producer exception");
    } else if (name == "consumer_error") {
        int n=0;
        throws([&] { runPipeline([&]()->std::optional<Reading> {
            if (n++==1000) return std::nullopt;
            return normal();
        },[](const Reading&) { throw std::runtime_error("Injected log/sink failure"); }); });
    } else if (name == "queue_close") {
        ThreadSafeQueue<int> queue; int v=0;
        require(queue.push(7),"Accept before close"); queue.close(); queue.close();
        require(!queue.push(8),"Reject after close");
        require(queue.pop(v) && v==7 && !queue.pop(v),"Drain then report end");
    } else if (name == "missing_file") {
        throws([&] { (void)loadReplay((temp.path/"absent.csv").string()); });
        throws([&] { (void)loadThresholds((temp.path/"absent.cfg").string()); });
    } else if (name == "replay") {
        auto p=temp.file("sample.csv","timestamp,heart_rate,oxygen,temperature,connected\r\n1767225600,75,,37,1\r\n1767225601,,,,0\r\n1767225602,-5,97,37,1\r\n");
        auto rows=loadReplay(p);
        require(rows.size()==3 && !rows[0].oxygen && !rows[1].connected,"CRLF and missing fields");
        require(validator.validate(rows[2])[0].type=="INVALID","Well-formed invalid value reaches validator");
    } else if (name == "nonfinite") {
        r.heartRate=std::numeric_limits<double>::quiet_NaN();
        r.oxygen=std::numeric_limits<double>::infinity();
        require(validator.validate(r).size()==2 && engine.evaluate(r).empty(),"Nonfinite values invalid and do not alert");
    } else throw std::runtime_error("Unknown test: " + name);
}
}
int main(int argc, char** argv) {
    try { require(argc==2,"Pass one test case name"); run(argv[1]); std::cout << "PASS " << argv[1] << '\n'; return 0; }
    catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
