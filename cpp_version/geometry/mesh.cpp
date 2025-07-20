#include "pch.h"
#include "mesh.h"

SubmeshCpp createSubmesh(
    vector<Vec3> positions,
    vector<Vec3> normals,
    vector<int> triangles,
    vector<int> faceIndices,
    int materialIndex
) {
    SubmeshCpp submesh;
    
    submesh.positions = (float*)malloc(3 * positions.size() * sizeof(float));
    submesh.normals = (float*)malloc(3 * normals.size() * sizeof(float));
    submesh.triangles = (int*)malloc(triangles.size() * sizeof(int));
    submesh.faceIndices = (int*)malloc(faceIndices.size() * sizeof(int));
    
    for (int i = 0; i < positions.size(); i++) {
        submesh.positions[3 * i + 0] = (float)positions[i].getX();
        submesh.positions[3 * i + 1] = (float)positions[i].getY();
        submesh.positions[3 * i + 2] = (float)positions[i].getZ();
    }
    for (int i = 0; i < normals.size(); i++) {
        submesh.normals[3 * i + 0] = (float)normals[i].getX();
        submesh.normals[3 * i + 1] = (float)normals[i].getY();
        submesh.normals[3 * i + 2] = (float)normals[i].getZ();
    }
    for (int i = 0; i < triangles.size(); i++) {
        submesh.triangles[i] = triangles[i];
    }
    for (int i = 0; i < faceIndices.size(); i++) {
        submesh.faceIndices[i] = faceIndices[i];
    }
    
    submesh.numVertices = (int)positions.size();
    submesh.numTriangles = (int)triangles.size() / 3;
    submesh.numFaces = (int)faceIndices.size();
    submesh.materialIndex = materialIndex;
    
    return submesh;
}

void freeMeshMemory(MeshCpp& mesh) {
    if (mesh.submeshes) {
        for (int i = 0; i < mesh.numSubmeshes; i++) {
            SubmeshCpp& submesh = mesh.submeshes[i];
            if (submesh.positions) {
                free(submesh.positions);
                submesh.positions = nullptr;
            }
            if (submesh.normals) {
                free(submesh.normals);
                submesh.normals = nullptr;
            }
            if (submesh.triangles) {
                free(submesh.triangles);
                submesh.triangles = nullptr;
            }
            if (submesh.faceIndices) {
                free(submesh.faceIndices);
                submesh.faceIndices = nullptr;
            }
        }
        free(mesh.submeshes);
        mesh.submeshes = nullptr;
    }
    mesh.numSubmeshes = 0;
}
