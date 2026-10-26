#include "vitalwatch/Pipeline.h"
#include "vitalwatch/ThreadSafeQueue.h"
#include <chrono>
#include <exception>
#include <thread>
namespace vitalwatch {
void runPipeline(const std::function<std::optional<Reading>()>& source,
                 const std::function<void(const Reading&)>& sink, unsigned intervalMs, std::size_t queueCapacity) {
    ThreadSafeQueue<Reading> queue(queueCapacity);
    std::exception_ptr producerError, consumerError;
    std::thread consumer([&] {
        try { Reading r; while (queue.pop(r)) sink(r); }
        catch (...) { consumerError = std::current_exception(); queue.close(); }
    });
    std::thread producer;
    try {
        producer = std::thread([&] {
            try {
                while (auto reading = source()) {
                    if (!queue.push(*reading)) break;
                    if (intervalMs) std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
                }
            } catch (...) { producerError = std::current_exception(); }
            queue.close();
        });
    } catch (...) { queue.close(); consumer.join(); throw; }
    producer.join(); consumer.join();
    if (consumerError) std::rethrow_exception(consumerError);
    if (producerError) std::rethrow_exception(producerError);
}
}
