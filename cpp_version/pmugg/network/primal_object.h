#pragma once

namespace ms {

class PrimalObject {
public:
    virtual ~PrimalObject() = default;

    // Pure virtual copy function to be implemented by derived classes
    virtual PrimalObject* copy() const = 0;
};

} // namespace ms 