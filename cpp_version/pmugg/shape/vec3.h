#pragma once
#include <string>
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

class Vec2;

class Vec3 {
public:
    Vec3(float x = 0, float y = 0, float z = 0);
    ~Vec3() = default;

    // Core accessors
    float getX() const { return x; }
    float getY() const { return y; }
    float getZ() const { return z; }
    float getValue(int dim) const;
    void setValue(float value, int dim);
    float operator[](int dim) const { return getValue(dim); }

    // Operations
    Vec3& add(const Vec3& v);
    Vec3& minus(const Vec3& v);
    Vec3& scale(float s);
    Vec3& normalize();
    float dot(const Vec3& v) const;
    Vec3 cross(const Vec3& v) const;
    float length() const;
    float length2() const;
    float distance(const Vec3& v) const;
    float distance2(const Vec3& v) const;
    Vec3& rotate(const Vec3& axis, float theta);
    Vec3& rotateX(float theta);
    Vec3& rotateY(float theta);
    Vec3& rotateZ(float theta);
    Vec3 copy() const;
    Vec2 dropDim(int dim) const;
    std::string toString() const;
    static Vec3 import(const Json & json);
    Vec3 swapAxes() const;

    // Static operations
    static Vec3 lerp(const Vec3& start, const Vec3& end, float s);
    static Vec3 unitVec(float theta, float phi);
    static bool coordinatesClose(const Vec3& a, const Vec3& b, float tolerance);

    // Operators
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3& operator+=(const Vec3& v) { return add(v); }
    Vec3& operator-=(const Vec3& v) { return minus(v); }
    Vec3& operator*=(float s) { return scale(s); }
    bool operator==(const Vec3& v) const { return x == v.x && y == v.y && z == v.z; }
    bool operator!=(const Vec3& v) const { return !(*this == v); }

    // Static constants
    static const Vec3 ORIGIN;
    static const Vec3 X_AXIS;
    static const Vec3 Y_AXIS;
    static const Vec3 Z_AXIS;

    float x;
    float y;
    float z;
};

} // namespace ms 