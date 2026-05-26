#include "pch.h"
#include "mesh_extraction/half_edge_mesh.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <unordered_map>

namespace mesh_extraction {

namespace {

// Computes a Newell-style polygon normal: stable for non-planar faces and
// signed by winding order. Returns a unit vector or (0,0,0) if degenerate.
Vec3d newellNormal(const ObjMesh& obj, const ObjMesh::Face& f) {
    double nx = 0, ny = 0, nz = 0;
    int n_corners = (int)f.corners.size();
    for (int i = 0; i < n_corners; ++i) {
        const auto& a = obj.vertices[f.corners[i].vertexIndex];
        const auto& b = obj.vertices[f.corners[(i + 1) % n_corners].vertexIndex];
        nx += (a.y - b.y) * (a.z + b.z);
        ny += (a.z - b.z) * (a.x + b.x);
        nz += (a.x - b.x) * (a.y + b.y);
    }
    double len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (len > 0) { nx /= len; ny /= len; nz /= len; }
    return Vec3d{nx, ny, nz};
}

// Encode an undirected vertex pair (a, b) into a single 64-bit key so we can
// hash-match a half-edge to its twin in O(N). The pair is unordered for twin
// lookup (twin of (a→b) is (b→a)) but we always use the directional key here
// because we WANT to find the unique opposite-direction half.
inline uint64_t directedKey(int from, int to) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(from)) << 32) |
            static_cast<uint64_t>(static_cast<uint32_t>(to));
}

}  // namespace

bool buildHalfEdgeMesh(const ObjMesh& obj,
                       HalfEdgeMesh&  out,
                       bool           useGroupAsVolume,
                       std::string*   error) {
    out = {};
    out.numVertices = (int)obj.vertices.size();
    out.vertexHalves.resize(obj.vertices.size());

    // --- Volume label assignment (step 2 of the plan). ----------------------
    // Map OBJ group name → volume id. The empty group counts as one volume.
    std::unordered_map<std::string, int> groupToVol;
    if (useGroupAsVolume) {
        for (const auto& f : obj.faces) {
            if (groupToVol.find(f.groupName) == groupToVol.end()) {
                int id = (int)groupToVol.size() + 1;  // 0 reserved for ambient
                groupToVol[f.groupName] = id;
                out.volumeNames.push_back(f.groupName);
            }
        }
    } else {
        // Single closed surface → one interior volume.
        groupToVol[""] = 1;
        out.volumeNames.push_back("interior");
    }
    out.numVolumes = (int)out.volumeNames.size() + 1;  // +1 for the ambient slot

    // --- Face creation + half-edge allocation. ------------------------------
    out.faces.reserve(obj.faces.size());
    for (size_t fi = 0; fi < obj.faces.size(); ++fi) {
        const ObjMesh::Face& srcFace = obj.faces[fi];
        if (srcFace.corners.size() < 3) {
            if (error) {
                char buf[128];
                snprintf(buf, sizeof(buf), "face %zu has %zu corners (< 3)",
                         fi, srcFace.corners.size());
                *error = buf;
            }
            return false;
        }

        HalfEdgeMesh::Face dstFace;
        dstFace.normal     = newellNormal(obj, srcFace);
        dstFace.materialId = srcFace.materialId;
        dstFace.group      = srcFace.groupName;

        int volInterior = useGroupAsVolume ? groupToVol[srcFace.groupName] : groupToVol[""];
        // Face normal points from "interior" into "ambient" by OBJ convention
        // (outward winding). volAbove (normal side) is therefore ambient (0)
        // and volBelow is the enclosed interior.
        dstFace.volAbove   = 0;
        dstFace.volBelow   = volInterior;

        // Allocate half-edges in CCW order around the face.
        int nCorners = (int)srcFace.corners.size();
        int firstHE = (int)out.halfEdges.size();
        for (int k = 0; k < nCorners; ++k) {
            HalfEdgeMesh::HalfEdge he;
            he.origin = srcFace.corners[k].vertexIndex;
            he.dest   = srcFace.corners[(k + 1) % nCorners].vertexIndex;
            he.face   = (int)fi;
            he.twin   = -1;
            he.next   = firstHE + (k + 1) % nCorners;
            he.prev   = firstHE + (k - 1 + nCorners) % nCorners;
            he.edge   = -1;  // populated in twin-matching pass
            out.halfEdges.push_back(he);
            dstFace.halfEdges.push_back(firstHE + k);
            out.vertexHalves[he.origin].push_back(firstHE + k);
        }
        out.faces.push_back(std::move(dstFace));
    }

    // --- Twin matching + edge creation. -------------------------------------
    // For each directed half (a→b) look for (b→a). If found exactly once, pair
    // them and create one edge owning both halves. Halves with no match are
    // boundary halves (flag the mesh as non-closed).
    std::unordered_map<uint64_t, int> dirKey;
    dirKey.reserve(out.halfEdges.size() * 2);
    for (int i = 0; i < (int)out.halfEdges.size(); ++i) {
        const auto& he = out.halfEdges[i];
        uint64_t key = directedKey(he.origin, he.dest);
        auto ins = dirKey.emplace(key, i);
        if (!ins.second) {
            // Duplicate directed half-edge — winding error or non-manifold.
            if (error) {
                char buf[160];
                snprintf(buf, sizeof(buf),
                         "duplicate directed edge %d→%d (faces %d, %d) — "
                         "non-manifold or inconsistent winding",
                         he.origin, he.dest, he.face,
                         out.halfEdges[ins.first->second].face);
                *error = buf;
            }
            return false;
        }
    }
    for (int i = 0; i < (int)out.halfEdges.size(); ++i) {
        auto& he = out.halfEdges[i];
        if (he.twin >= 0) continue;
        uint64_t twinKey = directedKey(he.dest, he.origin);
        auto it = dirKey.find(twinKey);
        if (it == dirKey.end()) {
            out.isClosedManifold = false;
            continue;  // boundary half — no twin
        }
        int j = it->second;
        if (i == j) continue;  // shouldn't happen but guard

        HalfEdgeMesh::Edge edge;
        edge.halfA = (i < j) ? i : j;
        edge.halfB = (i < j) ? j : i;
        int edgeIdx = (int)out.edges.size();
        out.edges.push_back(edge);

        out.halfEdges[i].twin = j;
        out.halfEdges[j].twin = i;
        out.halfEdges[i].edge = edgeIdx;
        out.halfEdges[j].edge = edgeIdx;
    }

    // Boundary halves (no twin) need an edge too, so vertex-type derivation
    // sees a consistent picture. They'll be flagged separately via Edge.halfB = -1.
    for (int i = 0; i < (int)out.halfEdges.size(); ++i) {
        if (out.halfEdges[i].edge >= 0) continue;
        HalfEdgeMesh::Edge edge;
        edge.halfA = i;
        edge.halfB = -1;
        int edgeIdx = (int)out.edges.size();
        out.edges.push_back(edge);
        out.halfEdges[i].edge = edgeIdx;
    }

    return true;
}

}  // namespace mesh_extraction
