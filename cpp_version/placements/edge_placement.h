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
    std::vector<int> vertexIds;
    Vec3 dir;
    RuleApplierSettings* settings;
    std::vector<int> constraints;
};
