#pragma once
#include "../shape/vec3.h"
#include "../util/range.h"
#include <vector>
#include <memory>

namespace ms {

class Edge;
class NetTransistorSettings;
class Range;

class EdgePlacement {
public:
    EdgePlacement(Edge* edge, int id, NetTransistorSettings* settings);
    ~EdgePlacement() = default;

    // Core functionality
    Edge* getEdge() const { return edge; }
    int getId() const { return id; }
    const std::vector<int>& getVertexIds() const { return vertexIds; }
    const Vec3& getDir() const { return dir; }
    NetTransistorSettings* getSettings() const { return settings; }
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
    Vec3 dir;
    NetTransistorSettings* settings;
    std::vector<int> constraints;
};

} // namespace ms 