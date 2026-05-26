#include "pch.h"
#include "mesh_extraction/type_extractor.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <sstream>
#include <unordered_map>

namespace mesh_extraction {

namespace {

inline int64_t quantize(double v, double eps) {
    // Round-half-to-even via std::llround keeps positive and negative scaled
    // alike. eps controls the grid; 1e-4 means 4 digits of resolution.
    return static_cast<int64_t>(std::llround(v / eps));
}

struct QuantVec3 {
    int64_t x, y, z;
    bool operator==(const QuantVec3& o) const { return x == o.x && y == o.y && z == o.z; }
};
struct QuantVec3Hash {
    size_t operator()(const QuantVec3& q) const {
        // 64-bit mix; collisions on box.obj-scale data are negligible.
        uint64_t h = static_cast<uint64_t>(q.x) * 0x9E3779B97F4A7C15ULL;
        h ^= static_cast<uint64_t>(q.y) + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
        h ^= static_cast<uint64_t>(q.z) + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
        return static_cast<size_t>(h);
    }
};
inline QuantVec3 q3(const Vec3d& v, double eps) {
    return {quantize(v.x, eps), quantize(v.y, eps), quantize(v.z, eps)};
}

// Face type key: (material, volAbove, volBelow, quantized normal).
struct FaceTypeKey {
    std::string material;
    int         volAbove;
    int         volBelow;
    QuantVec3   normal;
    bool operator==(const FaceTypeKey& o) const {
        return material == o.material && volAbove == o.volAbove
            && volBelow == o.volBelow && normal == o.normal;
    }
};
struct FaceTypeKeyHash {
    size_t operator()(const FaceTypeKey& k) const {
        size_t h = std::hash<std::string>{}(k.material);
        h ^= std::hash<int>{}(k.volAbove) + 0x9E3779B9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(k.volBelow) + 0x9E3779B9 + (h << 6) + (h >> 2);
        h ^= QuantVec3Hash{}(k.normal)    + 0x9E3779B9 + (h << 6) + (h >> 2);
        return h;
    }
};

// Edge type key: ordered face-type pair (left, right) + quantized direction.
// Left = halfA's face (canonical direction), right = halfB's face (or -1 for
// open-boundary edges).
struct EdgeTypeKey {
    int        leftFaceType;
    int        rightFaceType;  // -1 if boundary
    QuantVec3  dir;
    bool operator==(const EdgeTypeKey& o) const {
        return leftFaceType == o.leftFaceType
            && rightFaceType == o.rightFaceType
            && dir == o.dir;
    }
};
struct EdgeTypeKeyHash {
    size_t operator()(const EdgeTypeKey& k) const {
        size_t h = std::hash<int>{}(k.leftFaceType);
        h ^= std::hash<int>{}(k.rightFaceType) + 0x9E3779B9 + (h << 6) + (h >> 2);
        h ^= QuantVec3Hash{}(k.dir)            + 0x9E3779B9 + (h << 6) + (h >> 2);
        return h;
    }
};

// Walk CCW around `vertex` to enumerate outgoing half-edges. Uses the
// twin/next traversal: from any outgoing halfEdge h, the next outgoing
// halfEdge is `h.prev.twin` (rotate to the previous edge of the face, then
// flip across the shared edge to the next face). Requires a closed manifold.
//
// Returns the ordered list of outgoing-halfedge indices, or {} on failure
// (open boundary at this vertex, non-manifold fan, etc.).
std::vector<int> walkVertexFan(const HalfEdgeMesh& mesh, int vertexIdx) {
    const auto& outgoing = mesh.vertexHalves[vertexIdx];
    if (outgoing.empty()) return {};

    std::vector<int> fan;
    int start = outgoing.front();
    int cur = start;
    fan.push_back(cur);
    for (size_t guard = 0; guard < outgoing.size(); ++guard) {
        const auto& he = mesh.halfEdges[cur];
        // Step to previous edge of the same face, then cross via twin to the
        // next face. The result originates from the same vertex.
        int prev = he.prev;
        int twinOfPrev = mesh.halfEdges[prev].twin;
        if (twinOfPrev < 0) return {};  // open boundary — Phase B input
        int next = twinOfPrev;
        if (next == start) {
            // Sanity check: we should have visited every outgoing half exactly once.
            if (fan.size() != outgoing.size()) return {};
            return fan;
        }
        fan.push_back(next);
        cur = next;
    }
    return {};  // didn't close after |outgoing| steps — non-manifold
}

}  // namespace

bool extractTypes(const HalfEdgeMesh&         mesh,
                  const ObjMesh&              obj,
                  const TypeExtractionConfig& cfg,
                  ExtractedTypes&             out,
                  std::string*                error) {
    out = {};

    // --- Step 3: face types ------------------------------------------------
    out.faceTypeOfFace.resize(mesh.faces.size(), -1);
    std::unordered_map<FaceTypeKey, int, FaceTypeKeyHash> faceTypeMap;
    for (size_t fi = 0; fi < mesh.faces.size(); ++fi) {
        const auto& f = mesh.faces[fi];
        std::string mat;
        if (f.materialId >= 0 && f.materialId < (int)obj.materialNames.size())
            mat = obj.materialNames[f.materialId];

        FaceTypeKey key{mat, f.volAbove, f.volBelow, q3(f.normal, cfg.normalEps)};
        auto it = faceTypeMap.find(key);
        int idx;
        if (it == faceTypeMap.end()) {
            idx = (int)out.faceTypes.size();
            out.faceTypes.push_back({mat, f.normal, f.volAbove, f.volBelow});
            faceTypeMap.emplace(std::move(key), idx);
        } else {
            idx = it->second;
        }
        out.faceTypeOfFace[fi] = idx;
    }

    // --- Step 4: edge types ------------------------------------------------
    out.edgeTypeOfEdge.resize(mesh.edges.size(), -1);
    std::unordered_map<EdgeTypeKey, int, EdgeTypeKeyHash> edgeTypeMap;
    for (size_t ei = 0; ei < mesh.edges.size(); ++ei) {
        const auto& edge = mesh.edges[ei];
        const auto& hA = mesh.halfEdges[edge.halfA];
        const auto& vO = obj.vertices[hA.origin];
        const auto& vD = obj.vertices[hA.dest];
        double dx = vD.x - vO.x, dy = vD.y - vO.y, dz = vD.z - vO.z;
        double dlen = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (dlen > 0) { dx /= dlen; dy /= dlen; dz /= dlen; }
        Vec3d dir{dx, dy, dz};

        int leftFaceType  = out.faceTypeOfFace[hA.face];
        int rightFaceType = (edge.halfB >= 0)
            ? out.faceTypeOfFace[mesh.halfEdges[edge.halfB].face]
            : -1;

        EdgeTypeKey key{leftFaceType, rightFaceType, q3(dir, cfg.dirEps)};
        auto it = edgeTypeMap.find(key);
        int idx;
        if (it == edgeTypeMap.end()) {
            idx = (int)out.edgeTypes.size();
            ExtractedTypes::EdgeType et;
            et.dir = dir;
            et.faceData.push_back({leftFaceType,  /*onRight*/ false});
            if (rightFaceType >= 0)
                et.faceData.push_back({rightFaceType, /*onRight*/ true});
            out.edgeTypes.push_back(std::move(et));
            edgeTypeMap.emplace(std::move(key), idx);
        } else {
            idx = it->second;
        }
        out.edgeTypeOfEdge[ei] = idx;
    }

    // --- Step 5: vertex types (Phase A) ------------------------------------
    // For each input vertex, walk the fan and collect (edgeType, isAtStart).
    // isAtStart = true iff this half-edge is the canonical (halfA) direction
    // of its edge — equivalent to "the half-edge starts at this vertex in the
    // edge's stored orientation". Canonicalize by lexicographically smallest
    // cyclic rotation, then dedupe.
    out.vertexTypeOfVertex.resize(mesh.numVertices, -1);
    out.halfEdgeSlotInVertex.resize(mesh.halfEdges.size(), -1);

    struct CanonKey {
        std::vector<int64_t> tokens;  // alternating edgeTypeIdx, isAtStart{0|1}
        bool operator==(const CanonKey& o) const { return tokens == o.tokens; }
    };
    struct CanonKeyHash {
        size_t operator()(const CanonKey& k) const {
            size_t h = 0;
            for (auto t : k.tokens)
                h ^= std::hash<int64_t>{}(t) + 0x9E3779B9 + (h << 6) + (h >> 2);
            return h;
        }
    };

    std::unordered_map<CanonKey, int, CanonKeyHash> vertexTypeMap;
    for (int v = 0; v < mesh.numVertices; ++v) {
        if (mesh.vertexHalves[v].empty()) continue;  // unused vertex

        std::vector<int> fan = walkVertexFan(mesh, v);
        if (fan.empty()) {
            if (error) {
                char buf[160];
                snprintf(buf, sizeof(buf),
                         "vertex %d: cannot enumerate fan (open boundary or "
                         "non-manifold) — Phase B canonicalizer needed", v);
                *error = buf;
            }
            return false;
        }

        // Build raw token sequence (edgeType, isAtStart).
        struct Slot { int edgeType; int isAtStart; };
        std::vector<Slot> rawSlots;
        rawSlots.reserve(fan.size());
        for (int he : fan) {
            int edgeIdx = mesh.halfEdges[he].edge;
            int etIdx   = out.edgeTypeOfEdge[edgeIdx];
            // isAtStart = "this half-edge is the canonical (halfA) direction"
            bool isAtStart = (mesh.edges[edgeIdx].halfA == he);
            rawSlots.push_back({etIdx, isAtStart ? 1 : 0});
        }

        // Find canonical rotation (lexicographically smallest).
        int n = (int)rawSlots.size();
        int best = 0;
        for (int i = 1; i < n; ++i) {
            for (int k = 0; k < n; ++k) {
                const Slot& a = rawSlots[(best + k) % n];
                const Slot& b = rawSlots[(i + k) % n];
                if (a.edgeType != b.edgeType) {
                    if (b.edgeType < a.edgeType) best = i;
                    break;
                }
                if (a.isAtStart != b.isAtStart) {
                    if (b.isAtStart < a.isAtStart) best = i;
                    break;
                }
            }
        }

        CanonKey key;
        key.tokens.reserve(n * 2);
        for (int k = 0; k < n; ++k) {
            const Slot& s = rawSlots[(best + k) % n];
            key.tokens.push_back(s.edgeType);
            key.tokens.push_back(s.isAtStart);
        }

        auto it = vertexTypeMap.find(key);
        int vtIdx;
        if (it == vertexTypeMap.end()) {
            vtIdx = (int)out.vertexTypes.size();
            ExtractedTypes::VertexType vt;
            vt.halfEdgeTypes.reserve(n);
            for (int k = 0; k < n; ++k) {
                const Slot& s = rawSlots[(best + k) % n];
                vt.halfEdgeTypes.push_back({s.edgeType, s.isAtStart != 0});
            }
            out.vertexTypes.push_back(std::move(vt));
            vertexTypeMap.emplace(std::move(key), vtIdx);
        } else {
            vtIdx = it->second;
        }
        out.vertexTypeOfVertex[v] = vtIdx;

        // Record per-original-halfedge slot index in the canonical vertex type.
        // halfEdgeSlotInVertex[he] = position of `he` after rotation.
        for (int k = 0; k < n; ++k) {
            int srcIdx = (best + k) % n;
            int he     = fan[srcIdx];
            out.halfEdgeSlotInVertex[he] = k;
        }
    }

    return true;
}

}  // namespace mesh_extraction
