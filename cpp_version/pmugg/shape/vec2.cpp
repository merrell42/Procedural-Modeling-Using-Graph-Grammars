#include "pch.h"
#include "vec2.h"
#include "vec3.h"
#include <cmath>
#include <sstream>
#include <iomanip>

namespace ms {

const Vec2 Vec2::ORIGIN(0, 0);

Vec2::Vec2(float x_, float y_)
    : x(x_)
    , y(y_) {}

float Vec2::getValue(int dim) const {
    switch (dim) {
        case 0: return x;
        case 1: return y;
        default: throw std::runtime_error("Invalid dimension for Vec2");
    }
}

void Vec2::setValue(float value, int dim) {
    switch (dim) {
        case 0: x = value; break;
        case 1: y = value; break;
        default: throw std::runtime_error("Invalid dimension for Vec2");
    }
}

Vec2& Vec2::add(const Vec2& v) {
    x += v.x;
    y += v.y;
    return *this;
}

Vec2& Vec2::minus(const Vec2& v) {
    x -= v.x;
    y -= v.y;
    return *this;
}

Vec2& Vec2::scale(float s) {
    x *= s;
    y *= s;
    return *this;
}

Vec2& Vec2::normalize() {
    float len = length();
    if (len != 0) {
        scale(1.0f / len);
    }
    return *this;
}

float Vec2::dot(const Vec2& v) const {
    return x * v.x + y * v.y;
}

float Vec2::crossZ(const Vec2& v) const {
    return x * v.y - y * v.x;
}

float Vec2::length() const {
    return std::sqrt(length2());
}

float Vec2::length2() const {
    return x * x + y * y;
}

float Vec2::distance(const Vec2& v) const {
    return std::sqrt(distance2(v));
}

float Vec2::distance2(const Vec2& v) const {
    float dx = x - v.x;
    float dy = y - v.y;
    return dx * dx + dy * dy;
}

Vec2& Vec2::rotate(float theta) {
    float cosTheta = std::cos(theta);
    float sinTheta = std::sin(theta);
    float newX = x * cosTheta - y * sinTheta;
    float newY = x * sinTheta + y * cosTheta;
    x = newX;
    y = newY;
    return *this;
}

float Vec2::isLeft(const Vec2& p0, const Vec2& p1) const {
    return (p1.x - p0.x) * (y - p0.y) - (x - p0.x) * (p1.y - p0.y);
}

int Vec2::compare(const Vec2& other) const {
    if (x > other.x) return 1;
    if (x < other.x) return -1;
    if (y > other.y) return 1;
    if (y < other.y) return -1;
    return 0;
}

Vec2& Vec2::move(float dx, float dy) {
    x += dx;
    y += dy;
    return *this;
}

Vec2 Vec2::copy() const {
    return *this;
}

Vec2 Vec2::dropDim() const {
    return *this;
}

Vec3 Vec2::toVec3() const {
    return Vec3(x, y, 0);
}

std::string Vec2::toString() const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "(" << x << ", " << y << ")";
    return ss.str();
}

Vec2 Vec2::lerp(const Vec2& start, const Vec2& end, float s) {
    return Vec2(
        (1 - s) * start.x + s * end.x,
        (1 - s) * start.y + s * end.y
    );
}

Vec2 Vec2::unitVec(float angle) {
    return Vec2(std::cos(angle), std::sin(angle));
}

float Vec2::angle(const Vec2& start, const Vec2& end) {
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    return std::atan2(dy, dx);
}

bool Vec2::coordinatesClose(const Vec2& a, const Vec2& b, float tolerance) {
    return (std::abs(a.x - b.x) <= tolerance && 
            std::abs(a.y - b.y) <= tolerance);
}

Vec2 Vec2::import(const Json& json) {
    if (json.is_array()) {
        return Vec2(json[0], json[1]);
    } else {
        return Vec2(json["x"], json["y"]);
    }
}

} // namespace ms 