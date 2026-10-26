#pragma once
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <utility>

namespace vitalwatch {
template<class T> class ThreadSafeQueue {
    std::queue<T> items_;
    std::mutex mutex_;
    std::condition_variable ready_;
    std::condition_variable space_;
    const std::size_t capacity_; // Zero retains the original unbounded behavior.
    bool closed_ = false;
public:
    explicit ThreadSafeQueue(std::size_t capacity = 0) : capacity_(capacity) {}
    bool push(T value) {
        { std::unique_lock<std::mutex> lock(mutex_);
          space_.wait(lock, [this] { return closed_ || capacity_ == 0 || items_.size() < capacity_; });
          if (closed_) return false;
          items_.push(std::move(value)); }
        ready_.notify_one();
        return true;
    }
    bool tryPush(T value) {
        { std::lock_guard<std::mutex> lock(mutex_);
          if (closed_ || (capacity_ != 0 && items_.size() >= capacity_)) return false;
          items_.push(std::move(value)); }
        ready_.notify_one();
        return true;
    }
    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        ready_.wait(lock, [this] { return closed_ || !items_.empty(); });
        if (items_.empty()) return false;
        value = std::move(items_.front());
        items_.pop();
        lock.unlock();
        space_.notify_one();
        return true;
    }
    void close() {
        { std::lock_guard<std::mutex> lock(mutex_); closed_ = true; }
        ready_.notify_all();
        space_.notify_all();
    }
};
}
