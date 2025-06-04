#pragma once
#include "../geometry/vec3.h"
#include "../util/range.h"
#include <vector>
#include <memory>

namespace ms {

class Line;
class NetTransistorSettings;
class Range;

class EdgePlacement {
public:
    EdgePlacement(Line* edge, int id, NetTransistorSettings* settings);
    ~EdgePlacement() = default;

    Line* getEdge() const { return edge; }
    int getId() const { return id; }
    const std::vector<int>& getVertexIds() const { return vertexIds; }
    const Vec3& getDir() const { return dir; }
    NetTransistorSettings* getSettings() const { return settings; }
    const std::vector<int>& getConstraints() const { return constraints; }

    void initialize();
    void addConstraint(int id);
    Range getRange() const;

private:
    Line* edge;
    int id;
    std::vector<int> vertexIds;
    Vec3 dir;
    NetTransistorSettings* settings;
    std::vector<int> constraints;
};

} // namespace ms 