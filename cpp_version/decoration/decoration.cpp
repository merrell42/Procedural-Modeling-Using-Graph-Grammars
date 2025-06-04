#include "pch.h"
#include "decoration.h"

namespace ms {

Decoration::Decoration(const std::string& fillStyle, const std::string& strokeStyle,
                     const std::function<void()>& onChange)
    : fillStyle(fillStyle)
    , strokeStyle(strokeStyle)
    , onChange(onChange) {
}

void Decoration::set(const std::string& name, double value) {
    notifyChange();
}

void Decoration::set(const std::string& name, bool value) {
    notifyChange();
}

void Decoration::set(const std::string& name, const std::string& value) {
    notifyChange();
}

double Decoration::getDouble(const std::string& name) const {
    return 0.0f;
}

bool Decoration::getBool(const std::string& name) const {
    return false;
}

std::string Decoration::getString(const std::string& name) const {
    return "";
}

void Decoration::notifyChange() {
    if (onChange) {
        onChange();
    }
}

} // namespace ms 