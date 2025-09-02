#pragma once

#include <chrono>

class Timer {
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

  public:
    Timer() : start(Clock::now()) {}

    double elapsedMs() const {
        const auto end = Clock::now();
        const auto ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                .count();
        return double(ns) / 1'000'000;
    }

    void reset() { start = Clock::now(); }

  private:
    TimePoint start;
};
