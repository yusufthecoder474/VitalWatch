#include "vitalwatch/AlertEngine.h"
#include "vitalwatch/EventLogger.h"
#include "vitalwatch/Pipeline.h"
#include "vitalwatch/ReadingValidator.h"
#include "vitalwatch/Replay.h"
#include "vitalwatch/SensorSimulator.h"
#include "vitalwatch/ThreadSafeQueue.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace vitalwatch;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
constexpr std::int64_t base = 1767225600;
Reading sample(std::int64_t i = 0) { return {base+i,75,97,37,true}; }
struct Audit { std::size_t missing=0, duplicates=0, outOfOrder=0, unexpected=0; };
Audit audit(const std::vector<std::int64_t>& ids, std::size_t expected) {
    std::vector<bool> seen(expected,false);
    Audit result;
    for (std::size_t i=0; i<ids.size(); ++i) {
        auto id = ids[i]-base;
        if (i && ids[i]<ids[i-1]) ++result.outOfOrder;
        if (id<0 || static_cast<std::uint64_t>(id)>=expected) { ++result.unexpected; continue; }
        auto index = static_cast<std::size_t>(id);
        if (seen[index]) ++result.duplicates;
        seen[index]=true;
    }
    for (bool present : seen) if (!present) ++result.missing;
    return result;
}
void complete(const std::vector<std::int64_t>& ids, std::size_t expected) {
    auto a = audit(ids,expected);
    require(ids.size()==expected && !a.missing && !a.duplicates && !a.outOfOrder && !a.unexpected,
            "Missing, duplicate, out-of-order or unexpected sample");
}
std::string read(const std::filesystem::path& path) {
    std::ifstream in(path);
    require(static_cast<bool>(in),"Open output");
    std::ostringstream text; text << in.rdbuf(); return text.str();
}
template<class F> void exactError(F action, const char* expected) {
    try { action(); }
    catch (const std::runtime_error& e) { require(std::string(e.what())==expected,"Wrong propagated exception"); return; }
    throw std::runtime_error("Expected propagated runtime_error");
}
void run(const std::string& name) {
    if (name=="empty_pipeline") {
        std::size_t calls=0;
        runPipeline([]()->std::optional<Reading> { return std::nullopt; },[&](const Reading&) { ++calls; });
        require(calls==0,"Empty source must not invoke sink");
    } else if (name=="one_sample" || name=="large_fifo") {
        const std::size_t count = name=="one_sample" ? 1 : 10000;
        std::size_t produced=0;
        std::vector<std::int64_t> ids;
        runPipeline([&]()->std::optional<Reading> {
            if (produced==count) return std::nullopt;
            return sample(static_cast<std::int64_t>(produced++));
        },[&](const Reading& r) { ids.push_back(r.timestamp); });
        require(produced==count,"Generated count"); complete(ids,count);
        std::cout << "Generated=" << produced << " processed=" << ids.size()
                  << " missing=0 duplicates=0 out_of_order=0\n";
    } else if (name=="sequence_audit") {
        complete({base,base+1,base+2},3);
        auto duplicate=audit({base,base+1,base+1},3);
        require(duplicate.duplicates==1 && duplicate.missing==1,"Detect duplicate replacing a missing sample");
        require(audit({base,base+2},3).missing==1,"Detect dropped sample");
        require(audit({base+1,base,base+2},3).outOfOrder==1,"Detect reordered samples");
        require(audit({base,base+1,base+9},3).unexpected==1,"Detect unexpected ID");
    } else if (name=="queue_wait_close") {
        ThreadSafeQueue<Reading> queue;
        std::promise<void> entering;
        auto ready=entering.get_future();
        auto worker=std::async(std::launch::async,[&] {
            entering.set_value();
            Reading r;
            return queue.pop(r);
        });
        ready.get();
        // No sleep: close must work whether pop already waited or is about to wait.
        queue.close();
        require(worker.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Close did not release pop");
        require(!worker.get(),"Empty closed queue must return end-of-stream");
    } else if (name=="queued_drain") {
        std::promise<void> consumerEntered, sourceExhausted, releaseConsumer;
        auto entered=consumerEntered.get_future().share();
        auto exhausted=sourceExhausted.get_future();
        auto release=releaseConsumer.get_future().share();
        std::vector<std::int64_t> received;
        std::size_t produced=0;
        auto runner=std::async(std::launch::async,[&] {
            runPipeline([&]()->std::optional<Reading> {
                if (produced==1) entered.wait();
                if (produced==16) { sourceExhausted.set_value(); return std::nullopt; }
                return sample(static_cast<std::int64_t>(produced++));
            },[&](const Reading& r) {
                if (received.empty()) { consumerEntered.set_value(); release.wait(); }
                received.push_back(r.timestamp);
            });
        });
        auto status=exhausted.wait_for(std::chrono::seconds(5));
        // Release even if readiness failed, so cleanup does not strand the sink.
        releaseConsumer.set_value();
        runner.get();
        require(status==std::future_status::ready,"Producer did not finish with sink held");
        require(produced==16,"Expected producer count");
        complete(received,16); // Fifteen messages had been queued behind the held first callback.
    } else if (name=="exact_worker_errors") {
        int produced=0,processed=0;
        exactError([&] {
            runPipeline([&]()->std::optional<Reading> {
                if (produced==3) throw std::runtime_error("producer sentinel");
                return sample(produced++);
            },[&](const Reading&) { ++processed; });
        },"producer sentinel");
        require(produced==3 && processed==3,"Producer error must drain accepted work");
        std::size_t attempted=0;
        exactError([&] {
            runPipeline([&]()->std::optional<Reading> {
                if (attempted++==10000) return std::nullopt;
                return sample();
            },
                [](const Reading&) { throw std::runtime_error("consumer sentinel"); });
        },"consumer sentinel");
    } else if (name=="deterministic_replay") {
        auto rows=loadReplay(std::string(VITALWATCH_DATA_DIR)+"/sample_replay.csv");
        require(rows.size()==8,"Known replay fixture has eight rows");
        auto directory=std::filesystem::path("test-output-deterministic-replay");
        std::filesystem::remove_all(directory);
        ReadingValidator validator; AlertEngine alerts;
        auto process=[&](const std::string& mode) {
            EventLogger logger((directory/mode).string());
            SensorSimulator simulator(true,42,base);
            std::size_t index=0,eventCount=0;
            runPipeline([&]()->std::optional<Reading> {
                if (index==rows.size()) return std::nullopt;
                if (mode=="replay") return rows[index++];
                ++index; return simulator.next();
            },[&](const Reading& r) {
                auto events=validator.validate(r); auto extra=alerts.evaluate(r);
                events.insert(events.end(),extra.begin(),extra.end());
                eventCount+=events.size(); logger.write(r,events);
            });
            require(index==8 && eventCount==6,"Replay/demo sample and event totals");
        };
        process("replay"); process("generated");
        for (const auto* file : {"readings.csv","events.csv"})
            require(read(directory/"replay"/file)==read(directory/"generated"/file),"Replay/generated CSV equality");
        std::filesystem::remove_all(directory);
    } else throw std::runtime_error("Unknown case");
}
}
int main(int argc,char** argv) {
    try { require(argc==2,"Pass one test case name"); run(argv[1]); std::cout<<"PASS "<<argv[1]<<'\n'; return 0; }
    catch (const std::exception& e) { std::cerr<<"FAIL "<<e.what()<<'\n'; return 1; }
}
