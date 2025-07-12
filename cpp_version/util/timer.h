#pragma once
#include <string>
#include <chrono>
#include <map>
#include <vector>

using namespace std;
using Clock = chrono::high_resolution_clock;

const bool TIMER_ENABLED = true;

// Measures the time used in various processes.
class Timer {
public:
    Timer();
    ~Timer() = default;

    // Start and stop a timer with a particular name.
    void start(const string& name);
    void stop(const string& name);
    void reset();

    // Get the cumulative time used in a process.
    double getTime(const string& name) const;

    // Get the number of times a process was run.
    int getCount(const string& name) const;
    double getAverage(const string& name) const;

    // Print statistics from all the processes.
    void printStats() const;

    static Timer& instance();

private:
    struct TimerData {
        Clock::time_point startTime;
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

    // Ensure a timer exists for a given name.
    void ensureTimer(const string& name);
    static double toMilliseconds(const Clock::duration& duration);
};

extern Timer* timer;

// Convenience macro for timing code blocks
#define TIME_BLOCK(name) \
    ms::Timer::instance().start(name); \
    auto timer_guard = unique_ptr<void, function<void(void*)>>( \
        (void*)1, \
        [name](void*) { ms::Timer::instance().stop(name); } \
    )

