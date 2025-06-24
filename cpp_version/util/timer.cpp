#include "pch.h"
#include "timer.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "minmax.h"

Timer::Timer() = default;

Timer& Timer::instance() {
    static Timer instance;
    return instance;
}

void Timer::start(const string& name) {
    ensureTimer(name);
    auto& timer = timers[name];
    if (!timer.isRunning) {
        timer.startTime = chrono::high_resolution_clock::now();
        timer.isRunning = true;
    }
}

void Timer::stop(const string& name) {
    auto& timer = timers[name];
    if (timer.isRunning) {
        auto endTime = chrono::high_resolution_clock::now();
        auto duration = endTime - timer.startTime;
        timer.totalTime += toMilliseconds(duration);
        timer.count++;
        timer.isRunning = false;
    }
}

void Timer::reset() {
    timers.clear();
    order.clear();
}

double Timer::getTime(const string& name) const {
    auto it = timers.find(name);
    return (it != timers.end()) ? it->second.totalTime : 0.0;
}

int Timer::getCount(const string& name) const {
    auto it = timers.find(name);
    return (it != timers.end()) ? it->second.count : 0;
}

double Timer::getAverage(const string& name) const {
    auto it = timers.find(name);
    if (it != timers.end() && it->second.count > 0) {
        return it->second.totalTime / it->second.count;
    }
    return 0.0;
}

void Timer::printStats() const {
    if (!TIMER_ENABLED) {
        return;
    }

    // Calculate column widths
    size_t nameWidth = 0;
    size_t totalWidth = 12;  // Fixed width for total time
    size_t countWidth = 8;   // Fixed width for count
    size_t avgWidth = 12;    // Fixed width for average time

    for (const auto& [name, _] : timers) {
        nameWidth = max((int)nameWidth, (int)name.length());
    }
    nameWidth = max((int)nameWidth, (int)size_t(4));  // Minimum width for "Name"

    // Print header
    cout << left << setw(nameWidth) << "Name" << " | "
              << right << setw(totalWidth) << "Total (ms)" << " | "
              << setw(countWidth) << "Count" << endl;

    // Print separator
    cout << string(nameWidth + totalWidth + countWidth + avgWidth + 6, '-') << endl;

    // Sort timers by total time
    vector<pair<string, const TimerData*>> sortedTimers;
    for (const auto& [name, data] : timers) {
        sortedTimers.emplace_back(name, &data);
    }
    
    sort(sortedTimers.begin(), sortedTimers.end(),
        [](const auto& a, const auto& b) {
            return a.second->totalTime > b.second->totalTime;
        });

    // Print timer data
    for (const auto& [name, data] : sortedTimers) {
        double avg = data->count > 0 ? data->totalTime / data->count : 0.0;
        
        cout << left << setw(nameWidth) << name << " | "
                  << right << fixed << setprecision(3)
                  << setw(totalWidth) << data->totalTime << " | "
                  << setw(countWidth) << data->count << endl;
    }
}

void Timer::ensureTimer(const string& name) {
    if (timers.find(name) == timers.end()) {
        timers[name] = TimerData();
        order.push_back(name);
    }
}

double Timer::toMilliseconds(const chrono::high_resolution_clock::duration& duration) {
    return chrono::duration<double, milli>(duration).count();
}

Timer* timer = new Timer();

