#include "pch.h"
#include "decoration.h"



Decoration::Decoration(const string& fillStyle, const string& strokeStyle,
                     const function<void()>& onChange)
    : fillStyle(fillStyle)
    , strokeStyle(strokeStyle)
    , onChange(onChange) {
}

void Decoration::set(const string& name, double value) {
    notifyChange();
}

void Decoration::set(const string& name, bool value) {
    notifyChange();
}

void Decoration::set(const string& name, const string& value) {
    notifyChange();
}

double Decoration::getDouble(const string& name) const {
    return 0.0f;
}

bool Decoration::getBool(const string& name) const {
    return false;
}

string Decoration::getString(const string& name) const {
    return "";
}

void Decoration::notifyChange() {
    if (onChange) {
        onChange();
    }
}

