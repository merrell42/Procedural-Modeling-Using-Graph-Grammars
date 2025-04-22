#include "pch.h"
#include "mesh.h"

namespace ms {

Mesh createMesh(
    std::vector<Vec3> positions,
    std::vector<Vec3> normals,
    std::vector<int> triangles,
    std::vector<int> faceIndices
) {
    Mesh mesh;
    mesh.positions = (float*)malloc(3 * positions.size() * sizeof(float));
    mesh.normals = (float*)malloc(3 * normals.size() * sizeof(float));
    mesh.triangles = (int*)malloc(triangles.size() * sizeof(int));
    mesh.faceIndices = (int*)malloc(faceIndices.size() * sizeof(int));
    for (int i = 0; i < positions.size(); i++) {
        mesh.positions[3 * i + 0] = positions[i].getX();
        mesh.positions[3 * i + 1] = positions[i].getY();
        mesh.positions[3 * i + 2] = positions[i].getZ();
    }
    for (int i = 0; i < normals.size(); i++) {
        mesh.normals[3 * i + 0] = normals[i].getX();
        mesh.normals[3 * i + 1] = normals[i].getY();
        mesh.normals[3 * i + 2] = normals[i].getZ();
    }
    for (int i = 0; i < triangles.size(); i++) {
        mesh.triangles[i] = triangles[i];
    }
    for (int i = 0; i < faceIndices.size(); i++) {
        mesh.faceIndices[i] = faceIndices[i];
    }
    mesh.numVertices = positions.size();
    mesh.numTriangles = triangles.size() / 3;
    mesh.numFaces = faceIndices.size();
    return mesh;
}

}