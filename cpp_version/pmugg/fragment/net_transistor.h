#pragma once
#include "net.h"
#include "vec2.h"
#include <vector>
#include <memory>

namespace ms {

class NetTransistor {
public:
    NetTransistor(Net* net);
    virtual ~NetTransistor();

    // Core functionality
    void update();
    void draw(Context* context);
    bool isInside(const Vec2& point) const;
    void translate(const Vec2& delta);
    void rotate(float angle, const Vec2& center);
    void scale(float factor, const Vec2& center);
    NetTransistor* clone() const;

    // Transistor-specific methods
    void setGatePosition(const Vec2& position);
    void setSourcePosition(const Vec2& position);
    void setDrainPosition(const Vec2& position);
    Vec2 getGatePosition() const;
    Vec2 getSourcePosition() const;
    Vec2 getDrainPosition() const;
    void updateConnections();

private:
    Net* net;
    Vec2 gatePosition;
    Vec2 sourcePosition;
    Vec2 drainPosition;
    float width;
    float length;
    bool isNType;

    // Helper methods
    void drawTransistor(Context* context) const;
    void drawConnections(Context* context) const;
    bool isPointNearLine(const Vec2& point, const Vec2& start, 
                        const Vec2& end, float threshold) const;
    Vec2 calculateCenter() const;
};

} // namespace ms 