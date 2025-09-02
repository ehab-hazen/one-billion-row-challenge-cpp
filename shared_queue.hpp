#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class SharedQueue {
    public:
        SharedQueue() = default;
        ~SharedQueue() = default;

        SharedQueue(const SharedQueue&) = delete;
        SharedQueue& operator=(const SharedQueue&) = delete;

        SharedQueue(SharedQueue&&) = delete;
        SharedQueue& operator=(SharedQueue&&) = delete;

        void push(T val) {
            std::lock_guard lk(mtx);
            q.push(val);
            cv.notify_one();
        }

        T pop() {
            std::unique_lock lk(mtx);
            cv.wait(lk, [this]{ return !q.empty(); });
            T val = q.front();
            q.pop();
            return val;
        }

        size_t size() const {
            std::lock_guard lk(mtx);
            return q.size();
        }

        bool empty() const {
            std::lock_guard lk(mtx);
            return q.empty();
        }

    private:
        std::queue<T> q;
        mutable std::mutex mtx;
        mutable std::condition_variable cv;
};
