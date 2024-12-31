#include "line_state.h"
#include "cell.h"
#include "vertex.h"
#include "border.h"
#include "line_segment.h"
#include "line.h"
#include "face_connection.h"
#include "intersector.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace ms {

LineState::LineState(Stats* stats, const LineStateCoordinates& coords)
    : coordinates(coords)
    , dirty(true) {
    
    std::map<std::string, std::unique_ptr<Property>> properties;
    properties["cell"] = std::make_unique<AlternativeArray>("cell", false);
    properties["vertex"] = std::make_unique<RequiredArray>("vertex", false, 2);
    properties["border"] = std::make_unique<RequiredArray>("border", false, 2);
    properties["lineSegment"] = std::make_unique<SingleProperty>("lineSegment", true);
    properties["coordinates"] = std::make_unique<ValueProperty>("coordinates");

    node = std::make_unique<Node>(this, stats, "lineState", std::move(properties));
    node->setValue("coordinates", coordinates);
}

Node* LineState::getNode() const {
    return node.get();
}

std::vector<Cell*> LineState::getCells() const {
    return node->get("cell");
}

LineSegment* LineState::getSegment() const {
    return node->get("lineSegment");
}

std::vector<Vertex*> LineState::getVertices() const {
    return node->get("vertex");
}

std::vector<Border*> LineState::getBorders() const {
    return node->get("border");
}

Line* LineState::getLine() const {
    auto segment = getSegment();
    return segment ? segment->getLine() : nullptr;
}

const LineStateCoordinates& LineState::getCoordinates() const {
    return coordinates;
}

Vec2 LineState::getPosition(bool isAtStart) const {
    return coordinates.getAngledEdges()[isAtStart ? 0 : 1].getPosition();
}

float LineState::getAngle(bool isAtStart) const {
    return coordinates.getAngledEdges()[isAtStart ? 0 : 1].getAngle();
}

void LineState::setPosition(const Vec2& position, bool isAtStart) {
    auto edges = coordinates.edges;
    edges[isAtStart ? 0 : 1].position = position;
    coordinates = LineStateCoordinates(edges);
    dirty = true;
}

void LineState::move(const Vec2& movement0, const Vec2& movement1) {
    auto edges = coordinates.edges;
    edges[0].position += movement0;
    edges[1].position += movement1;
    coordinates = LineStateCoordinates(edges);
    refreshCells();
}

bool LineState::isMutable() const {
    auto cells = getCells();
    return cells.size() >= 2 && 
           cells.front()->isMutable() && 
           cells.back()->isMutable();
}

bool LineState::isPartiallyMutable(bool fromStart) const {
    auto cells = getCells();
    return !cells.empty() && 
           cells[fromStart ? 0 : cells.size() - 1]->isMutable();
}

void LineState::refreshCells() {
    auto oldCells = getCells();
    for (auto* cell : oldCells) {
        cell->removeState(this);
    }
    addToModel();
}

std::vector<LineState*> LineState::split(const Vec2& splitPoint) {
    auto edges = coordinates.edges;
    auto splitAngle = edges[0].angle + 
                     (edges[1].angle - edges[0].angle) * 
                     (splitPoint - edges[0].position).length() / 
                     (edges[1].position - edges[0].position).length();

    std::vector<LineState*> states;
    states.push_back(new LineState(node->getStats(), 
        LineStateCoordinates({
            edges[0],
            AngledEdge(splitPoint, splitAngle, 1)
        })));
    states.push_back(new LineState(node->getStats(), 
        LineStateCoordinates({
            AngledEdge(splitPoint, splitAngle, 0),
            edges[1]
        })));
    return states;
}

float LineState::getLength() const {
    return coordinates.getLength();
}

Vec2 LineState::getSplitPoint() const {
    return coordinates.getSplitPoint();
}

void LineState::addToModel() {
    forEachCell(true, [this](Cell* cell, float tLower, float tUpper) {
        if (cell) cell->addState(this);
        return nullptr;
    });
}

IntersectionResult LineState::addToModelWithIntersections(
    float minT, bool fromStart, bool trim,
    std::function<bool(LineState*)> isValidIntersection) {
    
    if (!isValidIntersection) {
        isValidIntersection = [](LineState*) { return true; };
    }

    std::vector<LineState*> checkedStates;
    auto u = fromStart ? this->u : this->u * -1.0f;
    float tIntersection = INFINITY;
    IntersectionResult firstIntersection = {Vec2(), nullptr};
    auto position0 = getPosition(fromStart);

    auto handleIntersection = [&](Vec2* intersection, LineState* state) {
        if (intersection && isValidIntersection(state)) {
            float t = getT(*intersection);
            if (!fromStart) {
                t = t1 - t;
            }
            if ((minT < t) && (t < tIntersection - 1e-4)) {
                tIntersection = t;
                firstIntersection = {*intersection, state};
            }
        }
    };

    forEachCell(fromStart, [&](Cell* cell, float tLower, float tUpper) {
        if (!cell || !cell->isMutable()) {
            float tNew = (tLower > Line::BACKTRACK * 1.1f) ? 
                        tLower - Line::BACKTRACK : 
                        tLower / 2;
            Vec2 newPosition = u * tNew + position0;
            setPosition(newPosition, !fromStart);

            auto border = cell ? cell->getMutationBorder() : nullptr;
            if (border) {
                border->addState(this, newPosition);
            }
            return new IntersectionResult{newPosition, nullptr};
        }

        for (auto* stateB : cell->getActiveStates()) {
            if (std::find(checkedStates.begin(), checkedStates.end(), stateB) == 
                checkedStates.end()) {
                checkedStates.push_back(stateB);
                handleIntersection(intersect(stateB), stateB);
            }
        }

        cell->addState(this);
        if (firstIntersection.state && tIntersection < tUpper) {
            if (trim) {
                setPosition(firstIntersection.position, !fromStart);
            }
            return new IntersectionResult(firstIntersection);
        }
        return nullptr;
    });

    return firstIntersection;
}

std::vector<FaceConnection*> LineState::findConnectionIntersections() const {
    std::set<FaceConnection*> faceConnections;
    forEachCell(true, [&](Cell* cell, float tLower, float tUpper) {
        if (cell) {
            auto connections = cell->getFaceConnections();
            faceConnections.insert(connections.begin(), connections.end());
        }
        return nullptr;
    });

    std::vector<FaceConnection*> result;
    std::copy_if(faceConnections.begin(), faceConnections.end(), 
                 std::back_inserter(result),
                 [this](FaceConnection* conn) {
                     return conn->intersectsState(*this);
                 });
    return result;
}

int LineState::countIntersections() const {
    std::vector<LineState*> checkedStates;
    int intersectionCount = 0;

    forEachCell(true, [&](Cell* cell, float tLower, float tUpper) {
        if (cell) {
            for (auto* stateB : cell->getActiveStates()) {
                if (std::find(checkedStates.begin(), checkedStates.end(), stateB) == 
                    checkedStates.end()) {
                    checkedStates.push_back(stateB);
                    if (auto intersection = intersect(stateB)) {
                        float t = getT(*intersection);
                        if ((1e-9 < t) && (t < t1 - 1e-9)) {
                            intersectionCount++;
                        }
                        delete intersection;
                    }
                }
            }
        }
        return nullptr;
    });

    return intersectionCount;
}

bool LineState::hasIntersections() const {
    std::vector<LineState*> checkedStates;

    return forEachCell(true, [&](Cell* cell, float tLower, float tUpper) {
        if (!cell) return nullptr;
        
        return cell->getActiveStates().end() != 
               std::find_if(cell->getActiveStates().begin(),
                          cell->getActiveStates().end(),
                          [&](LineState* stateB) {
                              if (std::find(checkedStates.begin(), checkedStates.end(), stateB) != 
                                  checkedStates.end()) {
                                  return false;
                              }
                              checkedStates.push_back(stateB);
                              if (auto intersection = intersect(stateB)) {
                                  float t = getT(*intersection);
                                  if ((1e-9 < t) && (t < t1 - 1e-9)) {
                                      delete intersection;
                                      return true;
                                  }
                                  delete intersection;
                              }
                              return false;
                          });
    }) != nullptr;
}

Vec2* LineState::intersect(LineState* other) const {
    if (this == other) return nullptr;

    auto thicknessA = getLine()->getEdgeType()->getThickness();
    auto thicknessB = other->getLine()->getEdgeType()->getThickness();
    auto intersection = coordinates.intersect(other->coordinates, thicknessA, thicknessB);

    if (intersection) {
        auto verticesA = getLine()->getEndpoints();
        auto verticesB = other->getLine()->getEndpoints();
        
        // The intersection does not count if the line states share a vertex
        if (verticesA[0] == verticesB[0] || verticesA[0] == verticesB[1] ||
            verticesA[1] == verticesB[0] || verticesA[1] == verticesB[1]) {
            delete intersection;
            return nullptr;
        }
    }

    return intersection;
}

void LineState::highlight(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen) {
    // Drawing implementation would go here
    // This depends on your graphics system
}

void LineState::print() const {
    std::cout << "LineState at positions: ";
    coordinates.edges[0].position.print();
    coordinates.edges[1].position.print();
}

void LineState::updateIfDirty() {
    if (!dirty) return;

    auto edges = coordinates.getAngledEdges();
    auto position0 = edges[0].getPosition();
    auto position1 = edges[1].getPosition();

    u = position1 - position0;
    u.normalize();
    v = Vec2(u.y, -u.x);
    u0 = u.dot(position0);
    u1 = u.dot(position1);
    t1 = u1 - u0;

    dirty = false;
}

float LineState::getT(const Vec2& point) const {
    const_cast<LineState*>(this)->updateIfDirty();
    return u.dot(point) - u0;
}

float LineState::lineDistance(const Vec2& point) const {
    const_cast<LineState*>(this)->updateIfDirty();
    return v.dot(point) - v.dot(getPosition(true));
}

CellTraverseResult LineState::forEachCell(
    bool fromStart,
    std::function<CellTraverseResult(Cell*, float, float)> func) {
    
    auto model = node->getModel();
    auto position = getPosition(fromStart);
    float x = position.x;
    float y = position.y;
    
    Vec2 u = fromStart ? this->u : this->u * -1.0f;
    int stepX = (u.x > 0) ? 1 : -1;
    int stepY = (u.y > 0) ? 1 : -1;
    float tDeltaX = std::abs(1 / u.x);
    float tDeltaY = std::abs(1 / u.y);
    
    int cellX = std::floor(x);
    int cellY = std::floor(y);

    float tMaxX = std::isfinite(tDeltaX) ?
        (stepX > 0 ? (std::ceil(x) - x) / u.x : (std::floor(x) - x) / u.x) :
        INFINITY;
    if (std::floor(x) == x) tMaxX = tDeltaX;

    float tMaxY = std::isfinite(tDeltaY) ?
        (stepY > 0 ? (std::ceil(y) - y) / u.y : (std::floor(y) - y) / u.y) :
        INFINITY;
    if (std::floor(y) == y) tMaxY = tDeltaY;

    float tLower = 0;
    while (true) {
        auto cell = model->getCell(cellX, cellY);
        float tUpper;
        
        if (tMaxX < tMaxY) {
            tUpper = tMaxX;
            tMaxX += tDeltaX;
            cellX += stepX;
        } else {
            tUpper = tMaxY;
            tMaxY += tDeltaY;
            cellY += stepY;
        }

        if (auto result = func(cell, tLower, tUpper)) {
            return result;
        }

        if (tUpper > t1) {
            return nullptr;
        }
        tLower = tUpper;
    }
}

float LineStateCoordinates::getLength() const {
    return (edges[1].position - edges[0].position).length();
}

Vec2 LineStateCoordinates::getSplitPoint() const {
    return Vec2::lerp(edges[0].position, edges[1].position, 
                     Util::random(0.3f, 0.7f));
}

Vec2* LineStateCoordinates::intersect(const LineStateCoordinates& other,
                                    float thicknessA, float thicknessB) const {
    return Intersector::intersect(edges[0].position, edges[1].position,
                                other.edges[0].position, other.edges[1].position,
                                thicknessA + thicknessB + LineState::INTERSECTION_THICKNESS);
}

} // namespace ms 