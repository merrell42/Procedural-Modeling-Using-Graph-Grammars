#pragma once
#include <map>
#include <string>

namespace ms {

class Counter {
public:
    Counter() = default;
    ~Counter() = default;

    // Operations
    void register_(const std::string& name);
    int add(const std::string& name);
    void reset();

private:
    std::map<std::string, int> props;
};

} // namespace ms 