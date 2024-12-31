#include "timer.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace ms {

Timer::Timer() = default;

Timer& Timer::instance() {
    static Timer instance;
    return instance;
}

void Timer::start(const std::string& name) {
    ensureTimer(name);
    auto& timer = timers[name];
    if (!timer.isRunning) {
        timer.startTime = std::chrono::high_resolution_clock::now();
        timer.isRunning = true;
    }
}

void Timer::stop(const std::string& name) {
    auto& timer = timers[name];
    if (timer.isRunning) {
        auto endTime = std::chrono::high_resolution_clock::now();
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

double Timer::getTime(const std::string& name) const {
    auto it = timers.find(name);
    return (it != timers.end()) ? it->second.totalTime : 0.0;
}

int Timer::getCount(const std::string& name) const {
    auto it = timers.find(name);
    return (it != timers.end()) ? it->second.count : 0;
}

double Timer::getAverage(const std::string& name) const {
    auto it = timers.find(name);
    if (it != timers.end() && it->second.count > 0) {
        return it->second.totalTime / it->second.count;
    }
    return 0.0;
}

void Timer::printStats() const {
    // Calculate column widths
    size_t nameWidth = 0;
    size_t totalWidth = 12;  // Fixed width for total time
    size_t countWidth = 8;   // Fixed width for count
    size_t avgWidth = 12;    // Fixed width for average time

    for (const auto& [name, _] : timers) {
        nameWidth = std::max(nameWidth, name.length());
    }
    nameWidth = std::max(nameWidth, size_t(4));  // Minimum width for "Name"

    // Print header
    std::cout << std::left << std::setw(nameWidth) << "Name" << " | "
              << std::right << std::setw(totalWidth) << "Total (ms)" << " | "
              << std::setw(countWidth) << "Count" << " | "
              << std::setw(avgWidth) << "Avg (ms)" << std::endl;

    // Print separator
    std::cout << std::string(nameWidth + totalWidth + countWidth + avgWidth + 6, '-') << std::endl;

    // Sort timers by total time
    std::vector<std::pair<std::string, const TimerData*>> sortedTimers;
    for (const auto& [name, data] : timers) {
        sortedTimers.emplace_back(name, &data);
    }
    
    std::sort(sortedTimers.begin(), sortedTimers.end(),
        [](const auto& a, const auto& b) {
            return a.second->totalTime > b.second->totalTime;
        });

    // Print timer data
    for (const auto& [name, data] : sortedTimers) {
        double avg = data->count > 0 ? data->totalTime / data->count : 0.0;
        
        std::cout << std::left << std::setw(nameWidth) << name << " | "
                  << std::right << std::fixed << std::setprecision(3)
                  << std::setw(totalWidth) << data->totalTime << " | "
                  << std::setw(countWidth) << data->count << " | "
                  << std::setw(avgWidth) << avg << std::endl;
    }
}

void Timer::ensureTimer(const std::string& name) {
    if (timers.find(name) == timers.end()) {
        timers[name] = TimerData();
        order.push_back(name);
    }
}

double Timer::toMilliseconds(const std::chrono::high_resolution_clock::duration& duration) {
    return std::chrono::duration<double, std::milli>(duration).count();
}

} // namespace ms 