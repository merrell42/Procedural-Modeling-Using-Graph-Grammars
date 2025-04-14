#include "pch.h"
#include "counter.h"

namespace ms {

void Counter::register_(const std::string& name) {
    props[name] = 0;
}

int Counter::add(const std::string& name) {
    return ++props[name];
}

void Counter::reset() {
    for (auto& [key, value] : props) {
        value = 0;
    }
}

} // namespace ms 