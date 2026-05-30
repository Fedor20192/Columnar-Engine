#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace cngn {

template <typename T>
class Queue {
public:
    explicit Queue(size_t capacity) : capacity_(capacity) {
    }

    bool Push(T &&value) {
        std::unique_lock lock(mutex_);

        cv_full_.wait(lock, [this]() { return queue_.size() < capacity_ || closed_; });

        if (closed_) {
            return false;
        }

        queue_.push(std::move(value));

        cv_empty_.notify_one();

        return true;
    }

    std::optional<T> Pop() {
        std::unique_lock lock(mutex_);

        cv_empty_.wait(lock, [this]() { return !queue_.empty() || closed_; });

        if (closed_ && queue_.empty()) {
            return std::nullopt;
        }

        T value = std::move(queue_.front());
        queue_.pop();

        cv_full_.notify_one();
        return value;
    }

    void Close() {
        if (closed_) {
            return;
        }
        {
            std::unique_lock lock(mutex_);
            closed_ = true;
        }
        cv_empty_.notify_all();
        cv_full_.notify_all();
    }

    ~Queue() {
        Close();
    }

private:
    bool closed_ = false;
    size_t capacity_;
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_full_;
    std::condition_variable cv_empty_;
};

}  // namespace cngn
