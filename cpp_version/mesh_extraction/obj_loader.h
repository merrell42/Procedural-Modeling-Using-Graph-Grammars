#pragma once
// Step 2.3.1 of MESH_TO_PRIMITIVES_PLAN.md: minimal .obj loader on top of
// tinyobjloader. The smoke-test consumer only reads counts; the struct is
// shaped to grow into the later type-derivation steps (vertex/edge/face
// type buckets need the per-face material, group, and corner indices).

#include <string>
#include <vector>

namespace mesh_extraction {

struct ObjMesh {
    struct Vec3 { float x, y, z; };

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

    std::vector<Vec3>         vertices;
    std::vector<Vec3>         normals;
    std::vector<Face>         faces;
    std::vector<std::string>  materialNames;  // index = materialId
};

// Returns true on success. On failure, fills `error` (if non-null) with the
// tinyobjloader error/warning text and leaves `out` partially populated.
bool loadObj(const char* path, ObjMesh& out, std::string* error = nullptr);

}  // namespace mesh_extraction
