#include "pch.h"
#include "vec3.h"
#include "vec2.h"
#include <cmath>
#include <sstream>
#include <iomanip>

namespace ms {

const Vec3 Vec3::ORIGIN(0, 0, 0);
const Vec3 Vec3::X_AXIS(1, 0, 0);
const Vec3 Vec3::Y_AXIS(0, 1, 0);
const Vec3 Vec3::Z_AXIS(0, 0, 1);

Vec3::Vec3(float x_, float y_, float z_)
    : x(x_)
    , y(y_)
    , z(z_) {}

float Vec3::getValue(int dim) const {
    switch (dim) {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        default: throw std::runtime_error("Invalid dimension for Vec3");
    }
}

void Vec3::setValue(float value, int dim) {
    switch (dim) {
        case 0: x = value; break;
        case 1: y = value; break;
        case 2: z = value; break;
        default: throw std::runtime_error("Invalid dimension for Vec3");
    }
}

Vec3& Vec3::add(const Vec3& v) {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

Vec3& Vec3::minus(const Vec3& v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

Vec3& Vec3::scale(float s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
}

Vec3& Vec3::normalize() {
    float len = length();
    if (len != 0) {
        scale(1.0f / len);
    }
    return *this;
}

float Vec3::dot(const Vec3& v) const {
    return x * v.x + y * v.y + z * v.z;
}

Vec3 Vec3::cross(const Vec3& v) const {
    return Vec3(
        y * v.z - z * v.y,
        z * v.x - x * v.z,
        x * v.y - y * v.x
    );
}

float Vec3::length() const {
    return std::sqrt(length2());
}

float Vec3::length2() const {
    return x * x + y * y + z * z;
}

float Vec3::distance(const Vec3& v) const {
    return std::sqrt(distance2(v));
}

float Vec3::distance2(const Vec3& v) const {
    float dx = x - v.x;
    float dy = y - v.y;
    float dz = z - v.z;
    return dx * dx + dy * dy + dz * dz;
}

Vec3& Vec3::rotate(const Vec3& axis, float theta) {
    float cosTheta = std::cos(theta);
    float sinTheta = std::sin(theta);
    float oneMinusCosTheta = 1.0f - cosTheta;

    float ux = axis.x;
    float uy = axis.y;
    float uz = axis.z;

    float newX = (cosTheta + ux * ux * oneMinusCosTheta) * x +
                 (ux * uy * oneMinusCosTheta - uz * sinTheta) * y +
                 (ux * uz * oneMinusCosTheta + uy * sinTheta) * z;

    float newY = (uy * ux * oneMinusCosTheta + uz * sinTheta) * x +
                 (cosTheta + uy * uy * oneMinusCosTheta) * y +
                 (uy * uz * oneMinusCosTheta - ux * sinTheta) * z;

    float newZ = (uz * ux * oneMinusCosTheta - uy * sinTheta) * x +
                 (uz * uy * oneMinusCosTheta + ux * sinTheta) * y +
                 (cosTheta + uz * uz * oneMinusCosTheta) * z;

    x = newX;
    y = newY;
    z = newZ;
    return *this;
}

Vec3& Vec3::rotateX(float theta) {
    float cosTheta = std::cos(theta);
    float sinTheta = std::sin(theta);
    float newY = y * cosTheta - z * sinTheta;
    float newZ = y * sinTheta + z * cosTheta;
    y = newY;
    z = newZ;
    return *this;
}

Vec3& Vec3::rotateY(float theta) {
    float cosTheta = std::cos(theta);
    float sinTheta = std::sin(theta);
    float newX = x * cosTheta + z * sinTheta;
    float newZ = -x * sinTheta + z * cosTheta;
    x = newX;
    z = newZ;
    return *this;
}

Vec3& Vec3::rotateZ(float theta) {
    float cosTheta = std::cos(theta);
    float sinTheta = std::sin(theta);
    float newX = x * cosTheta - y * sinTheta;
    float newY = x * sinTheta + y * cosTheta;
    x = newX;
    y = newY;
    return *this;
}

Vec3 Vec3::copy() const {
    return *this;
}

Vec2 Vec3::dropDim(int dim) const {
    if (dim == 0) {
        return Vec2(y, z);
    } else if (dim == 1) {
        return Vec2(x, z);
    } else {
        return Vec2(x, y);
    }
}

std::string Vec3::toString() const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "(" << x << ", " << y << ", " << z << ")";
    return ss.str();
}

Vec3 Vec3::lerp(const Vec3& start, const Vec3& end, float s) {
    return Vec3(
        (1 - s) * start.x + s * end.x,
        (1 - s) * start.y + s * end.y,
        (1 - s) * start.z + s * end.z
    );
}

Vec3 Vec3::unitVec(float theta, float phi) {
    float sinPhi = std::sin(phi);
    return Vec3(
        std::cos(theta) * sinPhi,
        std::sin(theta) * sinPhi,
        std::cos(phi)
    );
}

bool Vec3::coordinatesClose(const Vec3& a, const Vec3& b, float tolerance) {
    return (std::abs(a.x - b.x) <= tolerance && 
            std::abs(a.y - b.y) <= tolerance &&
            std::abs(a.z - b.z) <= tolerance);
}

Vec3 Vec3::swapAxes() const {
    return Vec3(z, x, y);
}

Vec3 Vec3::import(const Json& json) {
    if (json.is_array()) {
        return Vec3(json[0], json[1], json[2]);
    } else {
        return Vec3(json["x"], json["y"], json["z"]);
    }
}

} // namespace ms 