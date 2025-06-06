#include "pch.h"
#include "vec2.h"
#include "vec3.h"
#include <cmath>
#include <sstream>
#include <iomanip>



const Vec2 Vec2::ORIGIN(0, 0);

Vec2::Vec2(double x_, double y_)
    : x(x_), y(y_) {}

double Vec2::getValue(int dim) const {
    switch (dim) {
        case 0: return x;
        case 1: return y;
        default: throw runtime_error("Invalid dimension for Vec2");
    }
}

void Vec2::setValue(double value, int dim) {
    switch (dim) {
        case 0: x = value; break;
        case 1: y = value; break;
        default: throw runtime_error("Invalid dimension for Vec2");
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

Vec2& Vec2::scale(double s) {
    x *= s;
    y *= s;
    return *this;
}

Vec2& Vec2::normalize() {
    double len = length();
    if (len != 0) {
        scale(1.0f / len);
    }
    return *this;
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

double Vec2::distance(const Vec2& v) const {
    return sqrt(distance2(v));
}

double Vec2::distance2(const Vec2& v) const {
    double dx = x - v.x;
    double dy = y - v.y;
    return dx * dx + dy * dy;
}

Vec2& Vec2::rotate(double theta) {
    double cosTheta = cos(theta);
    double sinTheta = sin(theta);
    double newX = x * cosTheta - y * sinTheta;
    double newY = x * sinTheta + y * cosTheta;
    x = newX;
    y = newY;
    return *this;
}

double Vec2::isLeft(const Vec2& p0, const Vec2& p1) const {
    return (p1.x - p0.x) * (y - p0.y) - (x - p0.x) * (p1.y - p0.y);
}

int Vec2::compare(const Vec2& other) const {
    if (x > other.x) return 1;
    if (x < other.x) return -1;
    if (y > other.y) return 1;
    if (y < other.y) return -1;
    return 0;
}

Vec2& Vec2::move(double dx, double dy) {
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

string Vec2::toString() const {
    stringstream ss;
    ss << fixed << setprecision(3);
    ss << "(" << x << ", " << y << ")";
    return ss.str();
}

Vec2 Vec2::lerp(const Vec2& start, const Vec2& end, double s) {
    return Vec2(
        (1 - s) * start.x + s * end.x,
        (1 - s) * start.y + s * end.y
    );
}

Vec2 Vec2::unitVec(double angle) {
    return Vec2(cos(angle), sin(angle));
}

double Vec2::angle(const Vec2& start, const Vec2& end) {
    double dx = end.x - start.x;
    double dy = end.y - start.y;
    return atan2(dy, dx);
}

bool Vec2::coordinatesClose(const Vec2& a, const Vec2& b, double tolerance) {
    return (abs(a.x - b.x) <= tolerance && 
            abs(a.y - b.y) <= tolerance);
}

Vec2 Vec2::import(const Json& json) {
    if (json.is_array()) {
        return Vec2(json[0], json[1]);
    } else {
        return Vec2(json["x"], json["y"]);
    }
}

