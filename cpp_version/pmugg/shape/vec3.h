#pragma once
#include <string>
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

class Vec2;

class Vec3 {
public:
    Vec3(double x = 0, double y = 0, double z = 0);
    ~Vec3() = default;

    double getX() const { return x; }
    double getY() const { return y; }
    double getZ() const { return z; }
    double getValue(int dim) const;
    void setValue(double value, int dim);
    double operator[](int dim) const { return getValue(dim); }

    Vec3& add(const Vec3& v);
    Vec3& minus(const Vec3& v);
    Vec3& scale(double s);
    Vec3& normalize();
    double dot(const Vec3& v) const;
    Vec3 cross(const Vec3& v) const;
    double length() const;
    double length2() const;
    double distance(const Vec3& v) const;
    double distance2(const Vec3& v) const;
    Vec3& rotate(const Vec3& axis, double theta);
    Vec3& rotateX(double theta);
    Vec3& rotateY(double theta);
    Vec3& rotateZ(double theta);
    Vec3 copy() const;
    Vec2 dropDim(int dim) const;
    std::string toString() const;
    static Vec3 import(const Json & json);
    Vec3 swapAxes() const;

    static Vec3 lerp(const Vec3& start, const Vec3& end, double s);
    static Vec3 unitVec(double theta, double phi);
    static bool coordinatesClose(const Vec3& a, const Vec3& b, double tolerance);

    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(double s) const { return Vec3(x * s, y * s, z * s); }
    Vec3& operator+=(const Vec3& v) { return add(v); }
    Vec3& operator-=(const Vec3& v) { return minus(v); }
    Vec3& operator*=(double s) { return scale(s); }
    bool operator==(const Vec3& v) const { return x == v.x && y == v.y && z == v.z; }
    bool operator!=(const Vec3& v) const { return !(*this == v); }

    static const Vec3 ORIGIN;
    static const Vec3 X_AXIS;
    static const Vec3 Y_AXIS;
    static const Vec3 Z_AXIS;

private:
    double x;
    double y;
    double z;
};

} // namespace ms 