#include "net_transistor.h"
#include <cmath>

namespace ms {

NetTransistor::NetTransistor(Net* net)
    : net(net)
    , width(20.0f)
    , length(40.0f)
    , isNType(true) {
    
    // Initialize default positions
    gatePosition = Vec2(0, 0);
    sourcePosition = Vec2(0, -length/2);
    drainPosition = Vec2(0, length/2);
}

NetTransistor::~NetTransistor() = default;

void NetTransistor::update() {
    updateConnections();
}

void NetTransistor::draw(Context* context) {
    context->save();
    
    // Set drawing styles
    context->setStrokeStyle("#000");
    context->setLineWidth(1);
    
    // Draw the transistor components
    drawTransistor(context);
    drawConnections(context);
    
    context->restore();
}

bool NetTransistor::isInside(const Vec2& point) const {
    float threshold = 5.0f;  // Click detection threshold
    
    // Check if point is near any of the transistor lines
    return isPointNearLine(point, sourcePosition, gatePosition, threshold) ||
           isPointNearLine(point, gatePosition, drainPosition, threshold) ||
           isPointNearLine(point, Vec2(gatePosition.x - width/2, gatePosition.y),
                          Vec2(gatePosition.x + width/2, gatePosition.y), threshold);
}

void NetTransistor::translate(const Vec2& delta) {
    gatePosition += delta;
    sourcePosition += delta;
    drainPosition += delta;
    updateConnections();
}

void NetTransistor::rotate(float angle, const Vec2& center) {
    Vec2 c = calculateCenter();
    
    // Rotate each point around the center
    gatePosition = gatePosition.rotateAround(center, angle);
    sourcePosition = sourcePosition.rotateAround(center, angle);
    drainPosition = drainPosition.rotateAround(center, angle);
    
    updateConnections();
}

void NetTransistor::scale(float factor, const Vec2& center) {
    // Scale positions relative to center
    gatePosition = (gatePosition - center) * factor + center;
    sourcePosition = (sourcePosition - center) * factor + center;
    drainPosition = (drainPosition - center) * factor + center;
    
    // Scale dimensions
    width *= factor;
    length *= factor;
    
    updateConnections();
}

NetTransistor* NetTransistor::clone() const {
    auto* clone = new NetTransistor(net);
    clone->gatePosition = gatePosition;
    clone->sourcePosition = sourcePosition;
    clone->drainPosition = drainPosition;
    clone->width = width;
    clone->length = length;
    clone->isNType = isNType;
    return clone;
}

void NetTransistor::setGatePosition(const Vec2& position) {
    gatePosition = position;
    updateConnections();
}

void NetTransistor::setSourcePosition(const Vec2& position) {
    sourcePosition = position;
    updateConnections();
}

void NetTransistor::setDrainPosition(const Vec2& position) {
    drainPosition = position;
    updateConnections();
}

Vec2 NetTransistor::getGatePosition() const {
    return gatePosition;
}

Vec2 NetTransistor::getSourcePosition() const {
    return sourcePosition;
}

Vec2 NetTransistor::getDrainPosition() const {
    return drainPosition;
}

void NetTransistor::updateConnections() {
    if (net) {
        net->notifyChange();
    }
}

void NetTransistor::drawTransistor(Context* context) const {
    // Draw gate line
    context->beginPath();
    context->moveTo(gatePosition.x - width/2, gatePosition.y);
    context->lineTo(gatePosition.x + width/2, gatePosition.y);
    context->stroke();
    
    // Draw source and drain connections
    context->beginPath();
    context->moveTo(sourcePosition.x, sourcePosition.y);
    context->lineTo(gatePosition.x, gatePosition.y);
    context->lineTo(drainPosition.x, drainPosition.y);
    context->stroke();
    
    // Draw transistor type indicator (circle for NMOS, nothing for PMOS)
    if (isNType) {
        context->beginPath();
        context->arc(gatePosition.x, gatePosition.y, 3, 0, 2 * M_PI);
        context->stroke();
    }
}

void NetTransistor::drawConnections(Context* context) const {
    // Draw connection points
    float radius = 2.0f;
    
    context->beginPath();
    context->arc(sourcePosition.x, sourcePosition.y, radius, 0, 2 * M_PI);
    context->fill();
    
    context->beginPath();
    context->arc(drainPosition.x, drainPosition.y, radius, 0, 2 * M_PI);
    context->fill();
    
    context->beginPath();
    context->arc(gatePosition.x, gatePosition.y, radius, 0, 2 * M_PI);
    context->fill();
}

bool NetTransistor::isPointNearLine(const Vec2& point, const Vec2& start,
                                  const Vec2& end, float threshold) const {
    Vec2 line = end - start;
    Vec2 pointVec = point - start;
    
    float lineLengthSq = line.lengthSquared();
    if (lineLengthSq < 1e-6f) {
        return pointVec.length() <= threshold;
    }
    
    float t = std::clamp(Vec2::dot(pointVec, line) / lineLengthSq, 0.0f, 1.0f);
    Vec2 projection = start + line * t;
    
    return (point - projection).length() <= threshold;
}

Vec2 NetTransistor::calculateCenter() const {
    return (sourcePosition + gatePosition + drainPosition) / 3.0f;
}

} // namespace ms 