#pragma once
#include "../node/node.h"
#include "../shape/vec2.h"
#include <vector>
#include <memory>

namespace ms {

class Line;
class LineState;
class Vertex;
class Stats;

class LineSegment {
public:
    LineSegment(Stats* stats);
    ~LineSegment() = default;

    // Core functionality
    Node* getNode() const;
    std::vector<LineState*> getStates() const;
    LineState* getState(bool isAtStart) const;
    Line* getLine() const;
    float getLength(float idealLength);
    bool findIntersections();

    // State management
    void addState(LineState* state, bool insertAtStart = false);
    void addStates(const std::vector<LineState*>& states);
    void destroy();
    void merge(LineSegment* segmentB, LineState* mergedState);
    void split(LineState* unsplitState, const Vec2& splitPoint, Vertex* vertex, int index = -1);

    // Position operations
    void setPositionsOneState(const std::vector<Vec2>& positions);
    void setPositions(const std::vector<Vec2>& positions);
    void setPosition(const Vec2& newPosition, bool isAtStart);

    // Drawing
    void highlight(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen);
    void print() const;

    // Static members
    static bool moveMutable;

private:
    std::unique_ptr<Node> node;
    float lengthLowerBound;
    bool dirtyLength;

    // Helper methods
    void updateLength();
};

} // namespace ms 