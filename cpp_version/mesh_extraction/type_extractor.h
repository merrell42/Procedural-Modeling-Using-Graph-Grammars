#pragma once
// Steps 3-5 of MESH_TO_PRIMITIVES_PLAN.md: turn a half-edge mesh into the
// primitive type vectors that a pmugg grammar needs — face types, edge types,
// vertex types — plus per-element indices into those vectors.
//
// Outputs here are the canonical mesh→primitives schema in our own structs.
// Step 7 (seed_grammar_writer) turns this into pmugg-loadable JSON.

#include "mesh_extraction/half_edge_mesh.h"

#include <string>
#include <vector>

namespace mesh_extraction {

struct ExtractedTypes {
    // ----- Face types (step 3). One per unique (material, volAbove, volBelow,
    // normal_quantized). Identity preserves face-label semantics from §3.3
    // and the 3D extension from §7 (volume labels) of Merrell 2023.
    struct FaceType {
        std::string material;
        Vec3d       normal;
        int         volAbove;
        int         volBelow;
    };
    std::vector<FaceType> faceTypes;
    std::vector<int>      faceTypeOfFace;       // [faceIdx] = faceType index

    // ----- Edge types (step 4). One per unique (faceL, faceR, dir_quantized).
    // `dir` is the unit vector from the canonical halfA's origin to its dest.
    // `faceData[0]` is the face on the LEFT of `dir` (halfA's face) with
    // onRight=false; `faceData[1]` is the RIGHT face (halfB's face) with
    // onRight=true. Open-boundary edges omit the right-side entry.
    struct FaceData {
        int  faceType;   // index into faceTypes
        bool onRight;
    };
    struct EdgeType {
        std::vector<FaceData> faceData;
        Vec3d                 dir;
    };
    std::vector<EdgeType> edgeTypes;
    std::vector<int>      edgeTypeOfEdge;       // [edgeIdx] = edgeType index

    // ----- Vertex types (step 5, Phase A). One per unique canonical σ — the
    // cyclic CCW sequence of incident (edgeType, isAtStart) pairs around a
    // vertex. We pick the lexicographically smallest rotation as the canonical
    // form. This is the closed-manifold simplification of the paper's σ
    // (§3.3 end). Phase B (general 3D σ-graph) is a later TODO.
    struct HalfEdgeType {
        int  edgeType;   // index into edgeTypes
        bool isAtStart;  // true if this halfEdge originates at the vertex's "start"
    };
    struct VertexType {
        std::vector<HalfEdgeType> halfEdgeTypes;
    };
    std::vector<VertexType> vertexTypes;
    std::vector<int>        vertexTypeOfVertex;  // [vertIdx] = vertexType index

    // ----- Per-original-halfedge index into vertexType.halfEdgeTypes. Used
    // by step 6 (seed graph) to wire half-edge → vertex slot correctly.
    std::vector<int> halfEdgeSlotInVertex;
};

struct TypeExtractionConfig {
    double normalEps = 1e-4;   // quantization grid for face normals
    double dirEps    = 1e-4;   // quantization grid for edge directions
};

// Extracts primitives from the half-edge mesh. Currently implements Phase A
// (closed manifold with single-fan vertices). Returns false on inputs that
// require Phase B.
bool extractTypes(const HalfEdgeMesh&         mesh,
                  const ObjMesh&              obj,           // for material lookup
                  const TypeExtractionConfig& cfg,
                  ExtractedTypes&             out,
                  std::string*                error = nullptr);

}  // namespace mesh_extraction
