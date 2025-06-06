#include "pch.h"
#include "mesh.h"

Mesh createMesh(
    vector<Vec3> positions,
    vector<Vec3> normals,
    vector<int> triangles,
    vector<int> faceIndices
) {
    Mesh mesh;
    mesh.positions = (float*)malloc(3 * positions.size() * sizeof(float));
    mesh.normals = (float*)malloc(3 * normals.size() * sizeof(float));
    mesh.triangles = (int*)malloc(triangles.size() * sizeof(int));
    mesh.faceIndices = (int*)malloc(faceIndices.size() * sizeof(int));
    for (int i = 0; i < positions.size(); i++) {
        mesh.positions[3 * i + 0] = (float)positions[i].getX();
        mesh.positions[3 * i + 1] = (float)positions[i].getY();
        mesh.positions[3 * i + 2] = (float)positions[i].getZ();
    }
    for (int i = 0; i < normals.size(); i++) {
        mesh.normals[3 * i + 0] = (float)normals[i].getX();
        mesh.normals[3 * i + 1] = (float)normals[i].getY();
        mesh.normals[3 * i + 2] = (float)normals[i].getZ();
    }
    for (int i = 0; i < triangles.size(); i++) {
        mesh.triangles[i] = triangles[i];
    }
    for (int i = 0; i < faceIndices.size(); i++) {
        mesh.faceIndices[i] = faceIndices[i];
    }
    mesh.numVertices = (int)positions.size();
    mesh.numTriangles = (int)triangles.size() / 3;
    mesh.numFaces = (int)faceIndices.size();
    return mesh;
}
