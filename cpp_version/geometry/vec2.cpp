#include "pch.h"
#include "vec2.h"
#include "vec3.h"
#include <cmath>
#include <sstream>
#include <iomanip>

const Vec2 Vec2::ORIGIN(0, 0, false);

Vec2::Vec2(const Vec2& newVec2)
    : x(newVec2.x), y(newVec2.y) {
    MemoryCounter::creation("Vec2");
}

Vec2::~Vec2() {
    MemoryCounter::destruction("Vec2");
}

Vec2::Vec2(double x_, double y_, bool track)
    : x(x_), y(y_) {
    if (track) {
        MemoryCounter::creation("Vec2");
    }
}

double Vec2::dot(const Vec2& v) const {
    return x * v.x + y * v.y;
}

double Vec2::crossZ(const Vec2& v) const {
    return x * v.y - y * v.x;
}

double Vec2::length() const {
    return sqrt(length2());
}

double Vec2::length2() const {
    return x * x + y * y;
}

Vec2 Vec2::unitVec(double angle) {
    return Vec2(cos(angle), sin(angle));
}
