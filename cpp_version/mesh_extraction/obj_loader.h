#pragma once
// minimal .obj loader on top of tinyobjloader.
// Faces stay n-gons. Polylines (`l`) stay vertex chains so type extraction
// can label each segment with the face type on its left and its right.

#include <string>
#include <vector>

namespace mesh_extraction {

struct ObjMesh {
    struct Vec3 { float x, y, z; };
    struct Color { double x, y, z; };

    struct FaceCorner {
        int vertexIndex;    // 0-based
        int normalIndex;    // 0-based; -1 if absent
        int texCoordIndex;  // 0-based; -1 if absent
    };

    struct Face {
        std::vector<FaceCorner> corners;  // n-gon as authored; not triangulated
        int                     materialId;  // index into materialNames; -1 = none
        std::string             groupName;   // most recent `g` / `o` tag
    };

    // One `l` record. Indices are 0-based and keep the authored order,
    // including a repeated first index when the polyline is closed.
    struct Polyline {
        std::vector<int> vertexIndices;
        std::string      groupName;
    };

    std::vector<Vec3>         vertices;
    std::vector<Vec3>         normals;
    std::vector<Face>         faces;
    std::vector<Polyline>     polylines;
    std::vector<std::string>  materialNames;  // index = materialId
    std::vector<Color>        materialColors; // Kd from the .mtl; white if absent
};

// Returns true on success. On failure, fills `error` (if non-null) with the
// tinyobjloader error/warning text and leaves `out` partially populated.
bool loadObj(const char* path, ObjMesh& out, std::string* error = nullptr);

}  // namespace mesh_extraction
