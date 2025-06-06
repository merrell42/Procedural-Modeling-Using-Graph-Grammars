#pragma once

#include <vector>
#include "vec3.h"

using namespace std;

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
    vector<Vec3> positions,
    vector<Vec3> normals,
    vector<int> triangles,
    vector<int> faceIndices
);
