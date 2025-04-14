#pragma once

#include <vector>
#include "vec3.h"

namespace ms {

class Vec3;

// This is for exporting the graph drawing. It is not used internally.
struct Mesh {
    float* positions;
    int* triangles;
};

Mesh createMesh(std::vector<Vec3> positions, std::vector<int> triangles);

}