#pragma once
#include <string>

namespace Diagnostics {
    void clearWarning();
    void setWarning(const std::string& message);
    const std::string& getWarning();
}
