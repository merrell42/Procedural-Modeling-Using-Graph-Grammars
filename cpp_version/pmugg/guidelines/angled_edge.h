#pragma once
#include "vec2.h"

namespace ms {

class AngledEdge {
public:
    AngledEdge(const Vec2& position, float angle, float t);
    AngledEdge* copy() const;

    // Getters and setters
    Vec2 getPosition() const;
    void setPosition(const Vec2& position);
    float getAngle() const;
    void setAngle(float angle);
    float getT() const;
    void setT(float t);

    // Operations
    void set(const AngledEdge& crossing);
    void move(const Vec2& motion);
    std::string print() const;

    static constexpr float RAY_LENGTH = 1000.0f;

private:
    Vec2 position;
    float angle;
    // T is the coordinate along the direction of the edge tiling
    float t;
};

} // namespace ms 