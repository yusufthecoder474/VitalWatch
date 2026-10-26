#include "vitalwatch/Pipeline.h"
#include "vitalwatch/Replay.h"
#include "vitalwatch/SensorSimulator.h"
#include "vitalwatch/ThreadSafeQueue.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace vitalwatch;
namespace {
constexpr std::int64_t base = 1767225600;
void check(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
Reading reading(int index) { return {base+index,75,97,37,true}; }
template<class F> void exception(F action, const char* wanted) {
    try { action(); }
    catch (const std::runtime_error& e) { check(std::string(e.what()).find(wanted)!=std::string::npos,"Wrong error"); return; }
    throw std::runtime_error("Missing expected exception");
}
void run(const std::string& name) {
    if (name=="fw_malformed" || name=="fw_replay_mismatch") {
        auto dir=std::filesystem::path("test-output-"+name);
        std::filesystem::create_directories(dir);
        auto path=dir/"input.csv";
        auto write=[&](const std::string& row) {
            std::ofstream file(path);
            file<<"timestamp,heart_rate,oxygen,temperature,connected\n"<<row<<'\n';
            file.close(); check(static_cast<bool>(file),"Fixture write");
        };
        if (name=="fw_malformed") {
            for (const auto* row : {"1767225600,75junk,97,37,1","1767225600,75,97junk,37,1",
                                   "1767225600,75,97,37junk,1","1767225600,75,1e999,37,1"}) {
                write(row); exception([&] { (void)loadReplay(path.string()); },"Replay line 2:");
            }
        } else {
            SensorSimulator generator(true,42,base);
            auto expected=generator.next();
            write("1767225600,76,97,37,1");
            auto actual=loadReplay(path.string());
            check(actual.size()==1 && actual[0].timestamp==expected.timestamp,"Known replay sample");
            check(actual[0].heartRate!=expected.heartRate,"Known-reference value mismatch must be detected");
            check(actual[0].oxygen==expected.oxygen && actual[0].temperature==expected.temperature,
                  "Control must change only the intended value");
        }
        std::filesystem::remove_all(dir);
    } else if (name=="fw_capacity") {
        ThreadSafeQueue<int> queue(1);
        check(queue.tryPush(1) && !queue.tryPush(2),"Full capacity must reject nonblocking push");
        std::promise<void> entering; auto ready=entering.get_future();
        auto producer=std::async(std::launch::async,[&] { entering.set_value(); return queue.push(2); });
        ready.get(); int value=0;
        check(queue.pop(value) && value==1,"Retained first item");
        check(producer.get(),"Producer released by free capacity");
        queue.close();
        check(queue.pop(value) && value==2 && !queue.pop(value),"Second item and end of stream");
    } else if (name=="fw_close_saturated") {
        ThreadSafeQueue<int> queue(1); check(queue.push(1),"Initial push");
        std::promise<void> entering; auto ready=entering.get_future();
        auto producer=std::async(std::launch::async,[&] { entering.set_value(); return queue.push(2); });
        ready.get(); queue.close();
        check(!producer.get() && !queue.tryPush(3),"Close must release/reject pending producer");
        int value=0; check(queue.pop(value) && value==1 && !queue.pop(value),"Close retains accepted data");
    } else if (name=="fw_active_shutdown") {
        std::promise<void> threeAccepted, stopRequested;
        auto accepted=threeAccepted.get_future().share(); auto request=stopRequested.get_future().share();
        std::atomic<bool> stop{false}; int generated=0;
        std::vector<std::int64_t> done;
        runPipeline([&]()->std::optional<Reading> {
            if (generated==3) { threeAccepted.set_value(); request.wait(); }
            if (stop.load()) return std::nullopt;
            return reading(generated++);
        },[&](const Reading& r) {
            if (done.empty()) { accepted.wait(); stop.store(true); stopRequested.set_value(); }
            done.push_back(r.timestamp);
        },0,2);
        check(generated==3 && done==std::vector<std::int64_t>{base,base+1,base+2},
              "Stop during active callback must drain accepted work");
    } else if (name=="fw_bounded_exception") {
        int produced=0,processed=0;
        exception([&] { runPipeline([&]()->std::optional<Reading> {
            if (produced==3) throw std::runtime_error("producer fault");
            return reading(produced++);
        },[&](const Reading&) { ++processed; },0,1); },"producer fault");
        check(produced==3 && processed==3,"Producer fault drains bounded queue");
        int attempts=0;
        exception([&] { runPipeline([&]()->std::optional<Reading> {
            if (attempts==10000) return std::nullopt;
            return reading(attempts++);
        },[](const Reading&) { throw std::runtime_error("consumer fault"); },0,1); },"consumer fault");
    } else throw std::runtime_error("Unknown case");
}
void measure() {
    using Clock=std::chrono::steady_clock;
    constexpr std::size_t count=10000;
    std::vector<Clock::time_point> acquired(count);
    std::vector<double> latency; latency.reserve(count);
    std::size_t generated=0,processed=0;
    auto begin=Clock::now();
    runPipeline([&]()->std::optional<Reading> {
        if (generated==count) return std::nullopt;
        auto index=generated++;
        acquired[index]=Clock::now();
        return reading(static_cast<int>(index));
    },[&](const Reading& r) {
        auto now=Clock::now();
        check(r.timestamp==base+static_cast<std::int64_t>(processed),"Instrumented FIFO order");
        latency.push_back(std::chrono::duration<double,std::micro>(now-acquired[processed]).count());
        ++processed;
    },0,64);
    auto duration=std::chrono::duration<double>(Clock::now()-begin).count();
    check(generated==count && processed==count,"Instrumented sample count");
    std::sort(latency.begin(),latency.end());
    std::cout<<"samples,duration_s,samples_per_second,fifo_ok,latency_median_us,latency_p95_us,latency_max_us,capacity\n"
             <<std::setprecision(10)<<processed<<','<<duration<<','<<processed/duration<<",1,"
             <<(latency[4999]+latency[5000])/2<<','<<latency[9499]<<','<<latency.back()<<",64\n";
}
}
int main(int argc,char** argv) {
    try {
        check(argc==2,"Pass a named case or measure");
        if (std::string(argv[1])=="measure") { measure(); return 0; }
        run(argv[1]); std::cout<<"PASS "<<argv[1]<<'\n'; return 0;
    } catch (const std::exception& e) { std::cerr<<"FAIL "<<e.what()<<'\n'; return 1; }
}
