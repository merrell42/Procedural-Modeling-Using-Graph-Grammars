#include "pch.h"
#include "vec3.h"
#include "vec2.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>

const Vec3 Vec3::ORIGIN(0, 0, 0);
const Vec3 Vec3::X_AXIS(1, 0, 0);
const Vec3 Vec3::Y_AXIS(0, 1, 0);
const Vec3 Vec3::Z_AXIS(0, 0, 1);

Vec3::Vec3(double x_, double y_, double z_)
    : x(x_), y(y_), z(z_) {}

double Vec3::getValue(int dim) const {
    switch (dim) {
        case 0: return x;
        case 1: return y;
        case 2: return z;
        default:
            cout << "Invalid dimension" << endl;
            return 0;
    }
}

void Vec3::setValue(double value, int dim) {
    switch (dim) {
        case 0: x = value; break;
        case 1: y = value; break;
        case 2: z = value; break;
        default:
            cout << "Invalid dimension" << endl;
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

Vec3& Vec3::scale(double s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
}

Vec3& Vec3::normalize() {
    double len = length();
    if (len != 0) {
        scale(1.0f / len);
    }
    return *this;
}

double Vec3::dot(const Vec3& v) const {
    return x * v.x + y * v.y + z * v.z;
}

Vec3 Vec3::cross(const Vec3& v) const {
    return Vec3(
        y * v.z - z * v.y,
        z * v.x - x * v.z,
        x * v.y - y * v.x
    );
}

double Vec3::length() const {
    return sqrt(length2());
}

double Vec3::length2() const {
    return x * x + y * y + z * z;
}

double Vec3::distance(const Vec3& v) const {
    return sqrt(distance2(v));
}

double Vec3::distance2(const Vec3& v) const {
    double dx = x - v.x;
    double dy = y - v.y;
    double dz = z - v.z;
    return dx * dx + dy * dy + dz * dz;
}

Vec3& Vec3::rotate(const Vec3& axis, double theta) {
    double cosTheta = cos(theta);
    double sinTheta = sin(theta);
    double oneMinusCosTheta = 1.0f - cosTheta;

    double ux = axis.x;
    double uy = axis.y;
    double uz = axis.z;

    double newX = (cosTheta + ux * ux * oneMinusCosTheta) * x +
                 (ux * uy * oneMinusCosTheta - uz * sinTheta) * y +
                 (ux * uz * oneMinusCosTheta + uy * sinTheta) * z;

    double newY = (uy * ux * oneMinusCosTheta + uz * sinTheta) * x +
                 (cosTheta + uy * uy * oneMinusCosTheta) * y +
                 (uy * uz * oneMinusCosTheta - ux * sinTheta) * z;

    double newZ = (uz * ux * oneMinusCosTheta - uy * sinTheta) * x +
                 (uz * uy * oneMinusCosTheta + ux * sinTheta) * y +
                 (cosTheta + uz * uz * oneMinusCosTheta) * z;

    x = newX;
    y = newY;
    z = newZ;
    return *this;
}

Vec3& Vec3::rotateX(double theta) {
    double cosTheta = cos(theta);
    double sinTheta = sin(theta);
    double newY = y * cosTheta - z * sinTheta;
    double newZ = y * sinTheta + z * cosTheta;
    y = newY;
    z = newZ;
    return *this;
}

Vec3& Vec3::rotateY(double theta) {
    double cosTheta = cos(theta);
    double sinTheta = sin(theta);
    double newX = x * cosTheta + z * sinTheta;
    double newZ = -x * sinTheta + z * cosTheta;
    x = newX;
    z = newZ;
    return *this;
}

Vec3& Vec3::rotateZ(double theta) {
    double cosTheta = cos(theta);
    double sinTheta = sin(theta);
    double newX = x * cosTheta - y * sinTheta;
    double newY = x * sinTheta + y * cosTheta;
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

string Vec3::toString() const {
    stringstream ss;
    ss << fixed << setprecision(3);
    ss << "(" << x << ", " << y << ", " << z << ")";
    return ss.str();
}

Vec3 Vec3::lerp(const Vec3& start, const Vec3& end, double s) {
    return Vec3(
        (1 - s) * start.x + s * end.x,
        (1 - s) * start.y + s * end.y,
        (1 - s) * start.z + s * end.z
    );
}

Vec3 Vec3::unitVec(double theta, double phi) {
    double sinPhi = sin(phi);
    return Vec3(
        cos(theta) * sinPhi,
        sin(theta) * sinPhi,
        cos(phi)
    );
}

bool Vec3::coordinatesClose(const Vec3& a, const Vec3& b, double tolerance) {
    return (abs(a.x - b.x) <= tolerance && 
            abs(a.y - b.y) <= tolerance &&
            abs(a.z - b.z) <= tolerance);
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

