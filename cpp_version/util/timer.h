#pragma once
#include <string>
#include <chrono>
#include <map>
#include <vector>

using namespace std;

namespace ms {

class Timer {
public:
    Timer();
    ~Timer() = default;

    void start(const string& name);
    void stop(const string& name);
    void reset();

    double getTime(const string& name) const;
    int getCount(const string& name) const;
    double getAverage(const string& name) const;
    void printStats() const;

    static Timer& instance();

private:
    struct TimerData {
        chrono::high_resolution_clock::time_point startTime;
        double totalTime;
        int count;
        bool isRunning;

        TimerData()
            : totalTime(0)
            , count(0)
            , isRunning(false) {}
    };

    map<string, TimerData> timers;
    vector<string> order;

    void ensureTimer(const string& name);
    static double toMilliseconds(const chrono::high_resolution_clock::duration& duration);
};

extern Timer* timer;

// Convenience macro for timing code blocks
#define TIME_BLOCK(name) \
    ms::Timer::instance().start(name); \
    auto timer_guard = unique_ptr<void, function<void(void*)>>( \
        (void*)1, \
        [name](void*) { ms::Timer::instance().stop(name); } \
    )

} // namespace ms 