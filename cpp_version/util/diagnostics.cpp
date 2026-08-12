#include "pch.h"
#include "diagnostics.h"

namespace {
    std::string lastWarning;
}

void Diagnostics::clearWarning() {
    lastWarning.clear();
}

void Diagnostics::setWarning(const std::string& message) {
    if (lastWarning.empty()) {
        lastWarning = message;
    }
}

const std::string& Diagnostics::getWarning() {
    return lastWarning;
}
