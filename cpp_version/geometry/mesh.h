#pragma once

#include <vector>
#include "vec3.h"

using namespace std;

class Vec3;

// A set of faces with the same material.
struct SubmeshCpp {
    float* positions;
    float* normals;
    int* triangles;
    int* faceIndices;
    int numVertices;
    int numTriangles;
    int numFaces;
    int materialIndex;
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
    int materialIndex
);

void freeMeshMemory(MeshCpp& mesh);
