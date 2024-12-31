#pragma once
#include <string>
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

class Vec3;

class Vec2 {
public:
    Vec2(float x = 0, float y = 0);
    ~Vec2() = default;

    // Core accessors
    float getX() const { return x; }
    float getY() const { return y; }
    float getValue(int dim) const;
    void setValue(float value, int dim);

    // Operations
    Vec2& add(const Vec2& v);
    Vec2& minus(const Vec2& v);
    Vec2& scale(float s);
    Vec2& normalize();
    float dot(const Vec2& v) const;
    float crossZ(const Vec2& v) const;
    float length() const;
    float length2() const;
    float distance(const Vec2& v) const;
    float distance2(const Vec2& v) const;
    Vec2& rotate(float theta);
    float isLeft(const Vec2& p0, const Vec2& p1) const;
    int compare(const Vec2& other) const;
    Vec2& move(float dx, float dy);
    Vec2 copy() const;
    Vec2 dropDim() const;
    Vec3 toVec3() const;
    std::string toString() const;

    // Static operations
    static Vec2 lerp(const Vec2& start, const Vec2& end, float s);
    static Vec2 unitVec(float angle);
    static float angle(const Vec2& start, const Vec2& end);
    static bool coordinatesClose(const Vec2& a, const Vec2& b, float tolerance);

    // Operators
    Vec2 operator+(const Vec2& v) const { return Vec2(x + v.x, y + v.y); }
    Vec2 operator-(const Vec2& v) const { return Vec2(x - v.x, y - v.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    Vec2& operator+=(const Vec2& v) { return add(v); }
    Vec2& operator-=(const Vec2& v) { return minus(v); }
    Vec2& operator*=(float s) { return scale(s); }

    // Static constants
    static const Vec2 ORIGIN;

    static Vec2 import(const Json& json);

    float x;
    float y;
};

} // namespace ms 