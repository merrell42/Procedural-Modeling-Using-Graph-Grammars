#pragma once
#include <iostream>
#include <map>
#include <string>

// Memory counter is used to detect memory leaks.
class MemoryCounter {
public:
    static std::map<std::string, int> creationCounters;
    static std::map<std::string, int> destructionCounters;

    static void creation(const std::string& type);
    static void destruction(const std::string& type);
    static void printStats();
};