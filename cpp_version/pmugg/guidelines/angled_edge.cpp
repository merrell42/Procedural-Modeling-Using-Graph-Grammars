#include "angled_edge.h"

namespace ms {

AngledEdge::AngledEdge(const Vec2& position, float angle, float t)
    : position(position)
    , angle(angle)
    , t(t) {
}

AngledEdge* AngledEdge::copy() const {
    return new AngledEdge(position, angle, t);
}

Vec2 AngledEdge::getPosition() const {
    return position;
}

void AngledEdge::setPosition(const Vec2& newPosition) {
    position = newPosition;
}

float AngledEdge::getAngle() const {
    return angle;
}

void AngledEdge::setAngle(float newAngle) {
    angle = newAngle;
}

float AngledEdge::getT() const {
    return t;
}

void AngledEdge::setT(float newT) {
    t = newT;
}

void AngledEdge::set(const AngledEdge& crossing) {
    position = crossing.getPosition();
    angle = crossing.getAngle();
}

void AngledEdge::move(const Vec2& motion) {
    position += motion;
}

std::string AngledEdge::print() const {
    return position.print();
}

} // namespace ms 