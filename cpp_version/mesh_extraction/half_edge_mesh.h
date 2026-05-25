#pragma once
// Half-edge representation derived from an ObjMesh.
// Volume labels, face/edge/vertex type derivation all walk this structure.
// Geometry only — no pmugg types yet.
//
// We use a local Vec3d here instead of pmugg's geometry/vec3.h because this
// header is included by FPS_MapExporter, which has its own Vec3 in
// FPS_Math.h. The two types collide in any translation unit that sees both.

#include "mesh_extraction/obj_loader.h"

#include <vector>

namespace mesh_extraction {

struct Vec3d { double x, y, z; };

struct HalfEdgeMesh {
    struct HalfEdge {
        int origin;     // index of source vertex in ObjMesh::vertices
        int dest;       // index of destination vertex
        int face;       // index into faces (-1 = unbounded ambient side)
        int twin;       // index of twin half-edge (-1 if boundary)
        int next;       // next half-edge around `face` (CCW)
        int prev;       // prev half-edge around `face` (CCW)
        int edge;       // index into edges (each edge owns two halves)
    };

    struct Face {
        std::vector<int> halfEdges;   // ordered CCW (matches OBJ winding)
        Vec3d            normal;      // unit, area-weighted from the n-gon
        int              materialId;  // copy from ObjMesh::Face::materialId
        std::string      group;       // copy from ObjMesh::Face::groupName
        int              volAbove;    // volume on the normal-positive side
        int              volBelow;    // volume on the opposite side
    };

    struct Edge {
        int halfA;     // forward direction (origin < dest by some canonical order)
        int halfB;     // reverse direction (twin); -1 if mesh has a boundary
    };

    std::vector<HalfEdge>           halfEdges;
    std::vector<Face>               faces;
    std::vector<Edge>               edges;
    std::vector<std::vector<int>>   vertexHalves;  // vertexHalves[v] = halves originating at v
    int                              numVertices = 0;
    int                              numVolumes  = 0;  // 0 = ambient, 1..N = interior labels
    std::vector<std::string>         volumeNames;     // index = volumeId

    // Was every input edge shared by exactly two faces? Phase A of the plan
    // (closed-surface) requires this. If false, the input is open or
    // multi-manifold and the vertex-type Phase B canonicalizer will be needed.
    bool isClosedManifold = true;
};

// Build the half-edge structure from `obj`. Returns false on degenerate input
// (a face with < 3 corners, a non-manifold edge with 3+ incident faces, etc.).
// On failure, `error` describes the first problem encountered.
//
// Volume assignment:
//   - If `useGroupAsVolume` is true and the input has >1 distinct group, each
//     group gets its own volumeId (the group's interior). The ambient region
//     gets volumeId 0.
//   - Otherwise (or for single-group input), a single closed surface is
//     assumed: volumeId 0 = ambient, volumeId 1 = the enclosed interior.
bool buildHalfEdgeMesh(const ObjMesh& obj,
                       HalfEdgeMesh&  out,
                       bool           useGroupAsVolume,
                       std::string*   error = nullptr);

}  // namespace mesh_extraction
