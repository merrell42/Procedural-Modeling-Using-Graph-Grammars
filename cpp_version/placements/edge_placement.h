#pragma once
#include "../geometry/vec3.h"
#include "../util/range.h"
#include <vector>
#include <memory>

class Edge;
class RuleApplierSettings;
class Range;

class EdgePlacement {
public:
    EdgePlacement(Edge* edge, int id, RuleApplierSettings* settings);
    ~EdgePlacement() = default;

    Edge* getEdge() const;
    int getId() const;
    const vector<int>& getVertexIds() const;
    const Vec3& getDir() const;
    RuleApplierSettings* getSettings() const;
    const vector<int>& getConstraints() const;

    void initialize();
    void addConstraint(int id);
    Range getRange() const;

private:
    Edge* edge;
    int id;
    std::vector<int> vertexIds;
    Vec3 dir;
    RuleApplierSettings* settings;
    std::vector<int> constraints;
};
