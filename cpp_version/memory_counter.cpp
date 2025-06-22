#include "pch.h"
#include "memory_counter.h"

// This is used to detect memory leaks. It's not necessary to always have it on.
bool MemoryCounterEnabled = false;

std::map<std::string, int> MemoryCounter::creationCounters = {};
std::map<std::string, int> MemoryCounter::destructionCounters = {};
int finished = false;

void MemoryCounter::creation(const std::string& key) {
    if (!MemoryCounterEnabled) {
        return;
    }
    if (creationCounters.find(key) == creationCounters.end()) {
        creationCounters[key] = 0;
    }
    creationCounters[key]++;
}

void MemoryCounter::destruction(const std::string& key) {
    if (!MemoryCounterEnabled || finished) {
        return;
    }
    if (destructionCounters.find(key) == destructionCounters.end()) {
        destructionCounters[key] = 0;
    }
    destructionCounters[key]++;
}

void MemoryCounter::printStatistics() {
    if (!MemoryCounterEnabled) {
        return;
    }
    std::cout << "\n=== Memory Leak Detection Statistics ===" << std::endl;
    for (const auto& [key, value] : creationCounters) {
        int creationCount = value;
        int destructionCount = destructionCounters[key];
        int leakedCount = creationCount - destructionCount;
        std::cout << key << " Created: " << creationCount << ", Destroyed: " << destructionCount << ", Leaked: " << leakedCount << std::endl;
    }
    std::cout << "=========================================" << std::endl;
    finished = true;
} 