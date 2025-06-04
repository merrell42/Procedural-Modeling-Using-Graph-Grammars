#pragma once
#include <string>
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

class Vec3;

class Vec2 {
public:
    Vec2(double x = 0, double y = 0);
    ~Vec2() = default;
    static Vec2 import(const Json& json);

    double getX() const { return x; }
    double getY() const { return y; }
    double getValue(int dim) const;
    void setValue(double value, int dim);

    Vec2& add(const Vec2& v);
    Vec2& minus(const Vec2& v);
    Vec2& scale(double s);
    Vec2& normalize();
    double dot(const Vec2& v) const;
    double crossZ(const Vec2& v) const;
    double length() const;
    double length2() const;
    double distance(const Vec2& v) const;
    double distance2(const Vec2& v) const;
    Vec2& rotate(double theta);
    double isLeft(const Vec2& p0, const Vec2& p1) const;
    int compare(const Vec2& other) const;
    Vec2& move(double dx, double dy);
    Vec2 copy() const;
    Vec2 dropDim() const;
    Vec3 toVec3() const;
    std::string toString() const;

    static Vec2 lerp(const Vec2& start, const Vec2& end, double s);
    static Vec2 unitVec(double angle);
    static double angle(const Vec2& start, const Vec2& end);
    static bool coordinatesClose(const Vec2& a, const Vec2& b, double tolerance);

    Vec2 operator+(const Vec2& v) const { return Vec2(x + v.x, y + v.y); }
    Vec2 operator-(const Vec2& v) const { return Vec2(x - v.x, y - v.y); }
    Vec2 operator*(double s) const { return Vec2(x * s, y * s); }
    Vec2& operator+=(const Vec2& v) { return add(v); }
    Vec2& operator-=(const Vec2& v) { return minus(v); }
    Vec2& operator*=(double s) { return scale(s); }

    static const Vec2 ORIGIN;

    double x;
    double y;
};

} // namespace ms 