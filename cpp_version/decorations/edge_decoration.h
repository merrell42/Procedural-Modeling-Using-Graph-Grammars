#pragma once
#include <vector>
#include "../geometry/instance.h"
#include "../geometry/vec3.h"

using namespace std;

class EdgeDecoration {
public:
    virtual ~EdgeDecoration() = default;
    virtual vector<Instance> getInstances(const Vec3& start, const Vec3& end) const = 0;
};
