#pragma once

#include <vector>
#include "vec3.h"

using namespace std;

class Vec3;

// A set of faces with the same color.
struct SubmeshCpp {
    float* positions;
    float* normals;
    int* triangles;
    int* faceIndices;
    int numVertices;
    int numTriangles;
    int numFaces;
    float red;
    float green;
    float blue;
};
// This is for exporting the graph drawing. It is not used internally.
struct MeshCpp {
    SubmeshCpp* submeshes;
    int numSubmeshes;
};

SubmeshCpp createSubmesh(
    vector<Vec3> positions,
    vector<Vec3> normals,
    vector<int> triangles,
    vector<int> faceIndices,
    float red,
    float green,
    float blue
);

void freeMeshMemory(MeshCpp& mesh);
