#pragma once
#include <string>
#include "../third_party/json.h"
#include "../memory_counter.h"

using Json = nlohmann::json;
using namespace std;

class Vec3;

class Vec2 {
public:
    Vec2(const Vec2& newVec2);
    Vec2(double x = 0, double y = 0, bool track = true);
    ~Vec2() {
        MemoryCounter::destruction("Vec2");
    }

    double getX() const { return x; }
    double getY() const { return y; }

    double dot(const Vec2& v) const;
    double crossZ(const Vec2& v) const;
    double length() const;
    double length2() const;

    static Vec2 unitVec(double angle);

    Vec2 operator+(const Vec2& v) const { return Vec2(x + v.x, y + v.y); }
    Vec2 operator-(const Vec2& v) const { return Vec2(x - v.x, y - v.y); }
    Vec2 operator*(double s) const { return Vec2(x * s, y * s); }

    static const Vec2 ORIGIN;

    double x;
    double y;
};

