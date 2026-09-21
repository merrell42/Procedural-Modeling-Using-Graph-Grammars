#pragma once
#include <vector>
#include "../geometry/instance.h"
#include "../geometry/matrix4.h"

using namespace std;

class VertexDecoration {
public:
    virtual ~VertexDecoration() = default;
    virtual vector<Instance> getInstances(const Matrix4& transform) const = 0;
};
