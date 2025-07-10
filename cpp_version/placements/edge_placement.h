#pragma once
#include "../geometry/vec3.h"
#include "../util/range.h"
#include <vector>
#include <memory>
#include "../memory_counter.h"

class Edge;
class RuleApplierSettings;
class Range;

class EdgePlacement {
public:
    EdgePlacement(Edge* edge, int id, RuleApplierSettings* settings);
    ~EdgePlacement() {
        MemoryCounter::destruction("EdgePlacement");
    }

    void initialize();
    void addConstraint(int id);
    Range getRange() const;

private:
    Edge* edge;
    int id;
    Vec3 dir;
    RuleApplierSettings* settings;

    // The edges are constrainted by the vertices at the end of the edges.
    // Constraints is the same as vertex IDs, but may be in a different
    // order and only partially filled.
    std::vector<int> vertexIds;
    std::vector<int> constraints;
};
