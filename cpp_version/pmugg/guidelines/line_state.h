#pragma once
#include "../node/node.h"
#include "../shape/vec2.h"
#include "line_state_coordinates.h"
#include <vector>
#include <memory>
#include <functional>

namespace ms {

class Cell;
class Vertex;
class Border;
class LineSegment;
class Line;
class Stats;

class LineState {
public:
    static constexpr float INTERSECTION_THICKNESS = 1e-6f;

    LineState(Stats* stats, const LineStateCoordinates& coordinates);
    ~LineState() = default;

    // Core functionality
    Node* getNode() const;
    std::vector<Cell*> getCells() const;
    LineSegment* getSegment() const;
    std::vector<Vertex*> getVertices() const;
    std::vector<Border*> getBorders() const;
    Line* getLine() const;
    const LineStateCoordinates& getCoordinates() const;

    // Position operations
    Vec2 getPosition(bool isAtStart) const;
    float getAngle(bool isAtStart) const;
    void setPosition(const Vec2& position, bool isAtStart);
    void move(const Vec2& movement0, const Vec2& movement1);

    // State operations
    bool isMutable() const;
    bool isPartiallyMutable(bool fromStart) const;
    void refreshCells();
    std::vector<LineState*> split(const Vec2& splitPoint);
    float getLength() const;
    Vec2 getSplitPoint() const;

    // Model operations
    void addToModel();
    /*IntersectionResult addToModelWithIntersections(float minT = -INFINITY, bool fromStart = true, 
                                                 bool trim = true,
                                                 std::function<bool(LineState*)> isValidIntersection = nullptr);*/
    std::vector<FaceConnection*> findConnectionIntersections() const;
    int countIntersections() const;
    bool hasIntersections() const;
    Vec2* intersect(LineState* other) const;

    // Drawing
    void highlight(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen);
    void print() const;

private:
    std::unique_ptr<Node> node;
    LineStateCoordinates coordinates;
    bool dirty;
    Vec2 u;  // Direction vector
    Vec2 v;  // Normal vector
    float u0; // Projection of start point
    float u1; // Projection of end point
    float t1; // Total length parameter

    // Helper methods
    void updateIfDirty();
    float getT(const Vec2& point) const;
    float lineDistance(const Vec2& point) const;
    // CellTraverseResult forEachCell(bool fromStart, 
    //                               std::function<CellTraverseResult(Cell*, float, float)> func);
};

//struct LineStateCoordinates {
//    std::vector<AngledEdge> edges;
//    
//    LineStateCoordinates(const std::vector<AngledEdge>& edges = {}, float scale = 1.0f)
//        : edges(edges) {
//        for (auto& edge : this->edges) {
//            edge.position = edge.position * scale;
//        }
//    }
//
//    std::vector<AngledEdge> getAngledEdges() const { return edges; }
//    float getLength() const;
//    Vec2 getSplitPoint() const;
//    Vec2* intersect(const LineStateCoordinates& other, float thicknessA, float thicknessB) const;
//};

//struct AngledEdge {
//    Vec2 position;
//    float angle;
//    int index;
//
//    AngledEdge(const Vec2& pos, float ang, int idx)
//        : position(pos), angle(ang), index(idx) {}
//
//    Vec2 getPosition() const { return position; }
//    float getAngle() const { return angle; }
//};

struct IntersectionResult {
    Vec2 position;
    LineState* state;
};

using CellTraverseResult = IntersectionResult*;

} // namespace ms 