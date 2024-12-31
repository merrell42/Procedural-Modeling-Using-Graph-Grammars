#pragma once
#include "vec2.h"
#include <vector>
#include <memory>

namespace ms {

class Edge;
class PlacementSettings;
class Range;

class EdgePlacement {
public:
    EdgePlacement(Edge* edge, int id, PlacementSettings* settings);
    ~EdgePlacement() = default;

    // Core functionality
    Edge* getEdge() const { return edge; }
    int getId() const { return id; }
    const std::vector<int>& getVertexIds() const { return vertexIds; }
    const Vec2& getDir() const { return dir; }
    PlacementSettings* getSettings() const { return settings; }
    const std::vector<int>& getConstraints() const { return constraints; }

    // Operations
    void initialize();
    void addConstraint(int id);
    Range getRange() const;

    // Debug
    void print() const;

private:
    Edge* edge;
    int id;
    std::vector<int> vertexIds;
    Vec2 dir;
    PlacementSettings* settings;
    std::vector<int> constraints;
};

} // namespace ms 