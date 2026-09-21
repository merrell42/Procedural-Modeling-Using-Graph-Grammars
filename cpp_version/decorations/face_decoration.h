#pragma once
#include <vector>
#include "../geometry/instance.h"

using namespace std;

class Face;

class FaceDecoration {
public:
    virtual ~FaceDecoration() = default;
    virtual vector<Instance> getInstances(const Face& face) const = 0;
};
