#include "pch.h"
#include "memory_counter.h"

// Initialize static map
std::map<std::string, int> MemoryCounter::creationCounters = {};
std::map<std::string, int> MemoryCounter::destructionCounters = {};

void MemoryCounter::creation(const std::string& key) {
    if (creationCounters.find(key) == creationCounters.end()) {
        creationCounters[key] = 0;
    }
    creationCounters[key]++;
}

void MemoryCounter::destruction(const std::string& key) {
    if (destructionCounters.find(key) == destructionCounters.end()) {
        destructionCounters[key] = 0;
    }
    destructionCounters[key]++;
}

void MemoryCounter::printStatistics() {
    std::cout << "\n=== Memory Leak Detection Statistics ===" << std::endl;
    for (const auto& [key, value] : creationCounters) {
        int creationCount = value;
        int destructionCount = destructionCounters[key];
        int leakedCount = creationCount - destructionCount;
        std::cout << key << " Created: " << creationCount << ", Destroyed: " << destructionCount << ", Leaked: " << leakedCount << std::endl;
    }
    std::cout << "=========================================" << std::endl;
} 