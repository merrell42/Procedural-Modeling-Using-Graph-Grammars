#pragma once
#include <string>
#include <chrono>
#include <map>
#include <vector>

namespace ms {

class Timer {
public:
    Timer();
    ~Timer() = default;

    // Core operations
    void start(const std::string& name);
    void stop(const std::string& name);
    void reset();

    // Statistics
    double getTime(const std::string& name) const;
    int getCount(const std::string& name) const;
    double getAverage(const std::string& name) const;
    void printStats() const;

    // Static instance
    static Timer& instance();

private:
    struct TimerData {
        std::chrono::high_resolution_clock::time_point startTime;
        double totalTime;
        int count;
        bool isRunning;

        TimerData()
            : totalTime(0)
            , count(0)
            , isRunning(false) {}
    };

    std::map<std::string, TimerData> timers;
    std::vector<std::string> order;

    // Helper methods
    void ensureTimer(const std::string& name);
    static double toMilliseconds(const std::chrono::high_resolution_clock::duration& duration);
};

extern Timer* timer;

// Convenience macro for timing code blocks
#define TIME_BLOCK(name) \
    ms::Timer::instance().start(name); \
    auto timer_guard = std::unique_ptr<void, std::function<void(void*)>>( \
        (void*)1, \
        [name](void*) { ms::Timer::instance().stop(name); } \
    )

} // namespace ms 