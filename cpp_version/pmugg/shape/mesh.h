#pragma once

#include <vector>
#include "vec3.h"

namespace ms {

class Vec3;

// This is for exporting the graph drawing. It is not used internally.
struct Mesh {
    float* positions;
    float* normals;
    int* triangles;
    int* faceIndices;
    int numVertices;
    int numTriangles;
    int numFaces;
};

Mesh createMesh(
    std::vector<Vec3> positions,
    std::vector<Vec3> normals,
    std::vector<int> triangles,
    std::vector<int> faceIndices
);

}