#include "edge.h"
#include "../guidelines/vertex.h"
#include "../guidelines/endpoint.h"
// #include "shape.h"
// #include "view.h"
#include "../decoration/brush.h"
// #include "classifier.h"
#include "../util/util.h"
#include <cmath>

namespace ms {

int Edge::nextId = 0;

Edge::Edge(Vertex* startVertex, Vertex* endVertex)
    : start(std::make_unique<Endpoint>(this, startVertex, true))
    , end(std::make_unique<Endpoint>(this, endVertex, false))
    , shape(nullptr)
    , selected(false)
    , brush(nullptr)
    , id(nextId++)
    , key(id) {
    start->initialize();
    end->initialize();
}

void Edge::removeMe() {
    start->removeMe();
    end->removeMe();
}

bool Edge::isLeft(const Vertex* v) const {
    const Vec2& pos = v->getPosition();
    const Vec2& startPos = start->getPosition();
    const Vec2& endPos = end->getPosition();

    if ((startPos.y > pos.y) && (endPos.y > pos.y)) {
        return false;  // Edge entirely above v
    }
    if ((startPos.y < pos.y) && (endPos.y < pos.y)) {
        return false;  // Edge entirely below v
    }
    if (((startPos.y == pos.y) && (endPos.y < pos.y)) ||
        ((endPos.y == pos.y) && (startPos.y < pos.y))) {
        return false;  // Avoid double counting boundary
    }

    float d = horizontalDistance(v);
    return (d > 0) && (d < INFINITY);
}

float Edge::distanceToPoint(const Vertex* point) const {
    return lineDistanceToPoint(point);
}

float Edge::horizontalDistance(const Vertex* point) const {
    return lineHorizontalDistance(point);
}

float Edge::lineDistanceToPoint(const Vertex* point) const {
    const Vec2& p1 = start->getPosition();
    const Vec2& p2 = end->getPosition();
    const Vec2& p = point->getPosition();

    // Handle vertical line case
    if (p1.x == p2.x) {
        return std::abs(p1.x - p.x);
    }

    // Calculate slope and y-intercept
    float m = (p2.y - p1.y) / (p2.x - p1.x);
    float b = p1.y - m * p1.x;

    // Calculate distances
    std::vector<float> distances;
    
    // Distance to line
    distances.push_back(std::abs(p.y - m * p.x - b) / std::sqrt(m * m + 1));
    
    // Distance to endpoints
    distances.push_back((p - p1).length());
    distances.push_back((p - p2).length());

    return *std::min_element(distances.begin(), distances.end());
}

float Edge::lineHorizontalDistance(const Vertex* point) const {
    const Vec2& p1 = start->getPosition();
    const Vec2& p2 = end->getPosition();
    const Vec2& p = point->getPosition();

    float x = p1.x - (p1.y - p.y) / (p1.y - p2.y) * (p1.x - p2.x);
    return p.x - x;
}

/* Edge::Intercept Edge::intercept(const Vec2& position) const {
    return lineIntercept(position);
}

Edge::Intercept Edge::lineIntercept(const Vec2& position) const {
    const Vec2& p1 = start->getPosition();
    const Vec2& p2 = end->getPosition();

    if ((p1.y > position.y && p2.y > position.y) ||
        (p1.y < position.y && p2.y < position.y) ||
        ((p1.y == position.y) && (p2.y < position.y)) ||
        ((p2.y == position.y) && (p1.y < position.y))) {
        return {std::vector<Vertex*>(), std::vector<float>()};
    }

    float xResult = p1.x - (p1.y - position.y) / (p1.y - p2.y) * (p1.x - p2.x);
    auto* vertex = new Vertex(Vec2(xResult, position.y));
    
    return {
        std::vector<Vertex*>{vertex},
        std::vector<float>{tangent(0.5f)}
    };
} */

float Edge::arcLength() const {
    return start->distance(end.get());
}

int Edge::windingNumber(const Vertex* fillPoint, bool isLeftSide) const {
    int change = 0;
    std::vector<float> tangents;

    if (isLeft(fillPoint)) {
        tangents.push_back(start->tangent());
    }

    for (float t : tangents) {
        if (t != 0) {
            if ((t > 0) ? isLeftSide : !isLeftSide) {
                change++;
            } else {
                change--;
            }
        }
    }

    return change;
}

/* void Edge::draw(View* view, const Vec2& offset, const std::string& color, bool secondPass) {
    view->context.lineWidth = 3;
    view->context.beginPath();

    std::string drawColor = color.empty() ? 
        (brush ? brush->getColor() : COLOR) : color;
    bool flip = brush ? brush->get("Flip") : false;
    bool reverse = Classifier::switchDirections(this) ^ flip;

    DrawOptions options;
    options.color = drawColor;
    options.reverse = reverse;
    options.secondPass = secondPass;

    drawArea(view, offset, options);
    view->context.stroke();
} */

void Edge::drawArea(View* view, const Vec2& offset, const DrawOptions& options) {
    std::string color = options.color.empty() ? COLOR : options.color;
    if (selected) {
        color = SELECTED_COLOR;
    }

    auto* v1 = !options.reverse ? start.get() : end.get();
    auto* v2 = options.reverse ? start.get() : end.get();
    bool bidirectional = brush ? brush->get("Bidirectional Edges") : true;

    DrawOptions lineOptions = options;
    lineOptions.hasArrows = options.isArea || bidirectional ? 
        std::vector<bool>{false, false} : std::vector<bool>{false, true};
    lineOptions.brush = brush;

    view->drawLine(color, v1->getPosition(), v2->getPosition(), offset, lineOptions);
}

/* Edge::EdgeCoordinates Edge::edgeCoordinates(const Vec2& query) const {
    Vec2 startPos = start->getPosition();
    Vec2 endPos = end->getPosition();
    Vec2 u = endPos - startPos;
    u.normalize();
    Vec2 v(-u.y, u.x);

    float su = startPos.dot(u);
    float sv = startPos.dot(v);
    float qu = query.dot(u);
    float qv = query.dot(v);
    float eu = endPos.dot(u);

    return {
        (qu - su) / (eu - su),
        qv - sv
    };
} */

bool Edge::isClose(const Vec2& query, float scale) const {
    auto coords = edgeCoordinates(query);
    return (0 < coords.u) && (coords.u < 1) && 
           std::abs(coords.v) < Shape::NEAR_RADIUS / scale;
}

void Edge::split(Vertex* vertex) {
    auto* oldEnd = end.get();
    end = std::make_unique<Endpoint>(this, vertex, false);
    end->initialize();

    auto* newEdge = new Edge(vertex, oldEnd->getVertex());
    newEdge->setBrush(brush);
    shape->addEdge(newEdge);

    oldEnd->removeMe();
}

float Edge::tangent(float t) const {
    const Vec2& p1 = start->getPosition();
    const Vec2& p2 = end->getPosition();
    return std::atan2(p2.y - p1.y, p2.x - p1.x);
}

bool Edge::isAdjacent(const Edge* e1, const Edge* e2, const Vec2& offset1, const Vec2& offset2) {
    auto* a1 = e1->getStart();
    auto* b1 = e1->getEnd();
    auto* a2 = e2->getStart();
    auto* b2 = e2->getEnd();

    return (adjacentPoints(a1->getVertex(), a2->getVertex(), offset1, offset2) && 
            adjacentPoints(b1->getVertex(), b2->getVertex(), offset1, offset2)) ||
           (adjacentPoints(a1->getVertex(), b2->getVertex(), offset1, offset2) && 
            adjacentPoints(b1->getVertex(), a2->getVertex(), offset1, offset2));
}

bool Edge::adjacentPoints(const Vertex* v1, const Vertex* v2, const Vec2& offset1, const Vec2& offset2) {
    Vec2 p1 = v1->getPosition() + offset1;
    Vec2 p2 = v2->getPosition() + offset2;
    Vec2 diff = p1 - p2;
    return diff.dot(diff) < Shape::CONNECTION_DISTANCE2;
}

void Edge::print() const {
    std::cout << "Edge " << id << ": " 
              << start->getPosition().toString() << " -> " 
              << end->getPosition().toString() << std::endl;
}

int Edge::selectType() const {
    return Shape::SelectableTypes::EDGE;
}

} // namespace ms 