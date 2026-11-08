#include "pch.h"
#include "mesh_extraction/type_extractor.h"
#include "util/util.h"

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

int cmpQuant(const QuantVec3& a, const QuantVec3& b) {
    if (a.x != b.x) return (a.x < b.x) ? -1 : 1;
    if (a.y != b.y) return (a.y < b.y) ? -1 : 1;
    if (a.z != b.z) return (a.z < b.z) ? -1 : 1;
    return 0;
}

// Reversing an edge swaps its faces and negates its direction. Those two
// readings are the same type. Keep the lexicographically greater quantized
// direction. When the directions tie, keep the smaller (left, right) pair.
bool reversedIsCanonical(int left, int right, const QuantVec3& qdir, const QuantVec3& qneg) {
    if (right < 0) return false;
    int c = cmpQuant(qdir, qneg);
    if (c != 0) return c < 0;
    if (left != right) return right < left;
    return false;
}

// Edge type key: ordered face-type pair (left, right) + quantized direction,
// in the canonical orientation above. Right is -1 for an open boundary.
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

bool fanHasEdge(const HalfEdgeMesh& mesh, const std::vector<int>& fan, int edgeIdx) {
    for (int he : fan) {
        if (mesh.halfEdges[he].edge == edgeIdx) {
            return true;
        }
    }
    return false;
}

// Boundary edges have no twin, so the dest vertex has no outgoing half along
// that edge. A single-face corner (ground-plane rectangle) then looks like it
// has only one connection. Insert the incoming boundary half at each open end
// of the fan so both incident edges appear in the vertex type.
void addIncomingBoundaryHalves(
    const HalfEdgeMesh& mesh,
    int vertexIdx,
    std::vector<int>& fan
) {
    if (fan.empty()) {
        return;
    }
    auto maybeAdd = [&](int neighborHe, bool atFront) {
        int incoming = mesh.halfEdges[neighborHe].prev;
        if (incoming < 0) {
            return;
        }
        const auto& he = mesh.halfEdges[incoming];
        if (he.dest != vertexIdx) {
            return;
        }
        if (fanHasEdge(mesh, fan, he.edge)) {
            return;
        }
        if (atFront) {
            fan.insert(fan.begin(), incoming);
        } else {
            fan.push_back(incoming);
        }
    };
    maybeAdd(fan.front(), true);
    maybeAdd(fan.back(), false);
}

// Walk around `vertex` to enumerate incident half-edges in order.
// `h.prev.twin` and `h.twin.next` step opposite ways around the vertex.
// A closed umbrella loops back to the start. An open fan (boundary vertex
// after deleted faces, ProBoolean leftovers, etc.) hits a missing twin on
// both ends; we concatenate the two walks, then add the incoming boundary
// half at each open end. Returns {} if the incident halves are not a
// single fan.
std::vector<int> walkVertexFan(const HalfEdgeMesh& mesh, int vertexIdx) {
    const auto& outgoing = mesh.vertexHalves[vertexIdx];
    if (outgoing.empty()) return {};
    const int n = (int)outgoing.size();

    auto stepPrevTwin = [&](int h) -> int {
        return mesh.halfEdges[mesh.halfEdges[h].prev].twin;
    };
    auto stepTwinNext = [&](int h) -> int {
        int twin = mesh.halfEdges[h].twin;
        return twin < 0 ? -1 : mesh.halfEdges[twin].next;
    };

    const int start = outgoing.front();
    std::vector<char> seen(mesh.halfEdges.size(), 0);
    seen[start] = 1;

    std::vector<int> after;
    int cur = start;
    for (int i = 0; i < n; ++i) {
        int nxt = stepPrevTwin(cur);
        if (nxt < 0) break;
        if (mesh.halfEdges[nxt].origin != vertexIdx) return {};
        if (nxt == start) {
            if ((int)after.size() + 1 != n) return {};
            std::vector<int> fan;
            fan.reserve(n);
            fan.push_back(start);
            Util::append(fan, after);
            return fan;
        }
        if (seen[nxt]) return {};
        seen[nxt] = 1;
        after.push_back(nxt);
        cur = nxt;
    }

    std::vector<int> before;
    cur = start;
    for (int i = 0; i < n; ++i) {
        int nxt = stepTwinNext(cur);
        if (nxt < 0) break;
        if (mesh.halfEdges[nxt].origin != vertexIdx) return {};
        if (nxt == start || seen[nxt]) return {};
        seen[nxt] = 1;
        before.push_back(nxt);
        cur = nxt;
    }

    std::vector<int> fan;
    fan.reserve(before.size() + 1 + after.size() + 2);
    for (int i = (int)before.size() - 1; i >= 0; --i) fan.push_back(before[i]);
    fan.push_back(start);
    Util::append(fan, after);
    if ((int)fan.size() != n) return {};
    addIncomingBoundaryHalves(mesh, vertexIdx, fan);
    return fan;
}

// ProBoolean hole-with-a-bridge n-gons walk a slit both ways (house.obj:
// 1 7 6 11 … 14 11 6 2). That edge has the same face on both halves, so it
// is not a real connection. Counting it gives a vertex four half-edges
// when the mesh only has three.
bool isSlitEdge(const HalfEdgeMesh& mesh, int edgeIdx) {
    const auto& edge = mesh.edges[edgeIdx];
    if (edge.halfB < 0) return false;
    return mesh.halfEdges[edge.halfA].face == mesh.halfEdges[edge.halfB].face;
}

// Drop slit halves. Keep every other half, including two boundary edges of
// the same face — a ground-plane corner has one incident face and two
// connections. A pinched n-gon's slit is already not a real connection.
std::vector<int> dropSlitHalves(const HalfEdgeMesh& mesh, const std::vector<int>& fan) {
    std::vector<int> kept;
    kept.reserve(fan.size());
    for (int he : fan) {
        if (isSlitEdge(mesh, mesh.halfEdges[he].edge)) {
            continue;
        }
        kept.push_back(he);
    }
    return kept;
}

double dmin(double a, double b) { return a < b ? a : b; }
double dmax(double a, double b) { return a > b ? a : b; }

double dot3(const Vec3d& a, const Vec3d& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3d cross3(const Vec3d& a, const Vec3d& b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

Vec3d sub3(const Vec3d& a, const Vec3d& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

double len3(const Vec3d& a) { return std::sqrt(dot3(a, a)); }

Vec3d atVertex(const ObjMesh& obj, int index) {
    const auto& v = obj.vertices[index];
    return {v.x, v.y, v.z};
}

// Shared by mesh edges and polyline segments so both land on one canonical key.
struct InternedEdge {
    int  index;
    bool reversed;
};

InternedEdge internEdgeType(
    ExtractedTypes& out,
    std::unordered_map<EdgeTypeKey, int, EdgeTypeKeyHash>& edgeTypeMap,
    int leftFaceType,
    int rightFaceType,
    const Vec3d& dir,
    double dirEps
) {
    Vec3d negDir{-dir.x, -dir.y, -dir.z};
    QuantVec3 qdir = q3(dir, dirEps);
    QuantVec3 qneg = q3(negDir, dirEps);
    bool reversed = reversedIsCanonical(leftFaceType, rightFaceType, qdir, qneg);
    int keyLeft = reversed ? rightFaceType : leftFaceType;
    int keyRight = reversed ? leftFaceType : rightFaceType;

    EdgeTypeKey key{keyLeft, keyRight, reversed ? qneg : qdir};
    auto it = edgeTypeMap.find(key);
    int idx;
    if (it == edgeTypeMap.end()) {
        idx = (int)out.edgeTypes.size();
        ExtractedTypes::EdgeType et;
        et.dir = reversed ? negDir : dir;
        // The right-hand face is first. That is the slot the rules use.
        if (keyRight >= 0) {
            et.faceData.push_back({keyRight, /*onRight*/ true});
        }
        if (keyLeft >= 0) {
            et.faceData.push_back({keyLeft, /*onRight*/ false});
        }
        out.edgeTypes.push_back(std::move(et));
        edgeTypeMap.emplace(std::move(key), idx);
    } else {
        idx = it->second;
    }
    return {idx, reversed};
}

struct Vec2d { double x, y; };

double cross2(const Vec2d& a, const Vec2d& b) {
    return a.x * b.y - a.y * b.x;
}

double distPointSeg(const Vec2d& p, const Vec2d& a, const Vec2d& b) {
    double abx = b.x - a.x, aby = b.y - a.y;
    double len2 = abx * abx + aby * aby;
    if (len2 <= 1e-30) {
        return std::hypot(p.x - a.x, p.y - a.y);
    }
    double t = ((p.x - a.x) * abx + (p.y - a.y) * aby) / len2;
    if (t < 0.0) t = 0.0;
    else if (t > 1.0) t = 1.0;
    return std::hypot(p.x - (a.x + t * abx), p.y - (a.y + t * aby));
}

// Face projected into its own plane. `axisV` is `cross(normal, axisU)`, so a
// left turn in this 2D frame is a left turn when the face normal points at
// the viewer. That matches the half-edge winding: the face interior sits on
// the left of each boundary half-edge.
struct FaceGeom {
    bool valid = false;
    Vec3d normal{};
    Vec3d origin{};
    Vec3d axisU{};
    Vec3d axisV{};
    double planeTol = 0;
    std::vector<std::vector<Vec2d>> loops;
    std::vector<std::vector<int>>   loopVerts;
};

Vec3d tangentU(const Vec3d& n) {
    Vec3d a = (std::abs(n.x) <= std::abs(n.y) && std::abs(n.x) <= std::abs(n.z))
        ? Vec3d{1, 0, 0}
        : (std::abs(n.y) <= std::abs(n.z) ? Vec3d{0, 1, 0} : Vec3d{0, 0, 1});
    Vec3d u = cross3(n, a);
    double len = len3(u);
    if (len <= 1e-15) return {1, 0, 0};
    u.x /= len; u.y /= len; u.z /= len;
    return u;
}

double planeDist(const FaceGeom& g, const Vec3d& p) {
    return dot3(sub3(p, g.origin), g.normal);
}

Vec2d projectFace(const FaceGeom& g, const Vec3d& p) {
    Vec3d d = sub3(p, g.origin);
    return {dot3(d, g.axisU), dot3(d, g.axisV)};
}

FaceGeom buildFaceGeom(const HalfEdgeMesh& mesh, const ObjMesh& obj,
                       int faceIdx, double positionEps) {
    FaceGeom g;
    const auto& face = mesh.faces[faceIdx];
    g.normal = face.normal;
    if (len3(g.normal) <= 1e-8) return g;

    std::vector<char> seen(mesh.halfEdges.size(), 0);
    for (int start : face.halfEdges) {
        if (start < 0 || seen[start]) continue;
        std::vector<int> verts;
        int h = start;
        int guard = 0;
        do {
            if (h < 0 || seen[h] || guard++ > (int)mesh.halfEdges.size()) break;
            seen[h] = 1;
            verts.push_back(mesh.halfEdges[h].origin);
            h = mesh.halfEdges[h].next;
        } while (h != start);
        if ((int)verts.size() >= 3) g.loopVerts.push_back(std::move(verts));
    }
    if (g.loopVerts.empty()) return g;

    g.origin = atVertex(obj, g.loopVerts.front().front());
    g.axisU = tangentU(g.normal);
    g.axisV = cross3(g.normal, g.axisU);
    double vlen = len3(g.axisV);
    if (vlen <= 1e-15) return g;
    g.axisV.x /= vlen; g.axisV.y /= vlen; g.axisV.z /= vlen;

    double maxDev = 0;
    g.loops.reserve(g.loopVerts.size());
    for (const auto& verts : g.loopVerts) {
        std::vector<Vec2d> loop;
        loop.reserve(verts.size());
        for (int v : verts) {
            if (v < 0 || v >= (int)obj.vertices.size()) return FaceGeom{};
            Vec3d p = atVertex(obj, v);
            maxDev = dmax(maxDev, std::abs(planeDist(g, p)));
            loop.push_back(projectFace(g, p));
        }
        g.loops.push_back(std::move(loop));
    }
    // A bent face's own corners already leave the average plane. A polyline
    // on that face is allowed the same deviation, plus the snap tolerance.
    g.planeTol = maxDev + positionEps;
    g.valid = true;
    return g;
}

bool onBoundary2D(const FaceGeom& g, const Vec2d& p, double tol) {
    for (const auto& loop : g.loops) {
        int n = (int)loop.size();
        for (int i = 0; i < n; ++i) {
            if (distPointSeg(p, loop[i], loop[(i + 1) % n]) <= tol) return true;
        }
    }
    return false;
}

bool evenOddInside(const FaceGeom& g, const Vec2d& p) {
    int crossings = 0;
    for (const auto& loop : g.loops) {
        int n = (int)loop.size();
        for (int i = 0; i < n; ++i) {
            const Vec2d& a = loop[i];
            const Vec2d& b = loop[(i + 1) % n];
            if ((a.y > p.y) == (b.y > p.y)) continue;
            double t = (p.y - a.y) / (b.y - a.y);
            double x = a.x + t * (b.x - a.x);
            if (x > p.x) ++crossings;
        }
    }
    return (crossings & 1) == 1;
}

bool pointOnFace(const FaceGeom& g, const Vec3d& p, double tol) {
    if (!g.valid) return false;
    if (std::abs(planeDist(g, p)) > g.planeTol) return false;
    Vec2d p2 = projectFace(g, p);
    if (onBoundary2D(g, p2, tol)) return true;
    return evenOddInside(g, p2);
}

bool pointOnAnyFace(const std::vector<FaceGeom>& geoms, const Vec3d& p, double tol) {
    for (const auto& g : geoms) {
        if (pointOnFace(g, p, tol)) return true;
    }
    return false;
}

// None: the point only grazes a corner and does not own a side.
// Left / Right: the point lies on a boundary half-edge parallel to `dir`.
// The face interior is on the left of that half-edge, so a segment walking
// the same way has this face on its left.
// Both: the point is in the face interior.
enum class FaceSide { None, Left, Right, Both };

FaceSide sideAt(const FaceGeom& g, const ObjMesh& obj, const Vec2d& p2,
                const Vec2d& seg2, const Vec3d& dir, double tol) {
    bool onLeft = false;
    bool onRight = false;
    double slen = std::hypot(seg2.x, seg2.y);
    for (size_t li = 0; li < g.loopVerts.size(); ++li) {
        const auto& verts = g.loopVerts[li];
        const auto& loop = g.loops[li];
        int n = (int)verts.size();
        for (int i = 0; i < n; ++i) {
            const Vec2d& c = loop[i];
            const Vec2d& d = loop[(i + 1) % n];
            double elen = std::hypot(d.x - c.x, d.y - c.y);
            if (elen <= tol || slen <= tol) continue;
            double cr = (d.x - c.x) * seg2.y - (d.y - c.y) * seg2.x;
            if (std::abs(cr) / (elen * slen) > 1e-3) continue;
            if (distPointSeg(p2, c, d) > tol) continue;

            Vec3d e0 = atVertex(obj, verts[i]);
            Vec3d e1 = atVertex(obj, verts[(i + 1) % n]);
            Vec3d edge = sub3(e1, e0);
            double el3 = len3(edge);
            if (el3 <= tol) continue;
            double s = dot3(edge, dir) / el3;
            if (s >= 0.0) onLeft = true;
            else onRight = true;
        }
    }
    if (onLeft && onRight) return FaceSide::Both;
    if (onLeft) return FaceSide::Left;
    if (onRight) return FaceSide::Right;
    if (evenOddInside(g, p2)) return FaceSide::Both;
    return FaceSide::None;
}

void addSegmentCuts(const Vec2d& a, const Vec2d& b, const Vec2d& c, const Vec2d& d,
                    double tol, std::vector<double>& ts) {
    Vec2d r{b.x - a.x, b.y - a.y};
    Vec2d s{d.x - c.x, d.y - c.y};
    double rxs = cross2(r, s);
    double rlen = std::hypot(r.x, r.y);
    double slen = std::hypot(s.x, s.y);
    if (rlen <= tol || slen <= tol) return;
    Vec2d qp{c.x - a.x, c.y - a.y};
    if (std::abs(rxs) <= tol * dmax(rlen, slen)) {
        if (std::abs(cross2(qp, r)) > tol * rlen) return;
        double rr = r.x * r.x + r.y * r.y;
        double t0 = (qp.x * r.x + qp.y * r.y) / rr;
        double t1 = ((d.x - a.x) * r.x + (d.y - a.y) * r.y) / rr;
        double lo = dmax(0.0, dmin(t0, t1));
        double hi = dmin(1.0, dmax(t0, t1));
        if (hi > lo) {
            ts.push_back(lo);
            ts.push_back(hi);
        }
        return;
    }
    double t = cross2(qp, s) / rxs;
    double u = cross2(qp, r) / rxs;
    double tPad = tol / rlen;
    double uPad = tol / slen;
    if (t < -tPad || t > 1.0 + tPad || u < -uPad || u > 1.0 + uPad) return;
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    ts.push_back(t);
}

enum class SampleKind { Off, Conflict, Ready };

struct SampledSide {
    SampleKind kind;
    int left;
    int right;
};

SampledSide sampleSides(
    const ObjMesh& obj,
    const std::vector<FaceGeom>& geoms,
    const std::vector<int>& candidates,
    const std::vector<int>& faceTypeOfFace,
    const Vec3d& segA,
    const Vec3d& segB,
    const Vec3d& point,
    const Vec3d& dir,
    double tol
) {
    bool haveLeft = false;
    bool haveRight = false;
    int left = -1;
    int right = -1;
    bool hit = false;
    for (int fi : candidates) {
        const FaceGeom& g = geoms[fi];
        if (!pointOnFace(g, point, tol)) continue;
        Vec2d a2 = projectFace(g, segA);
        Vec2d b2 = projectFace(g, segB);
        Vec2d p2 = projectFace(g, point);
        FaceSide side = sideAt(g, obj, p2, {b2.x - a2.x, b2.y - a2.y}, dir, tol);
        if (side == FaceSide::None) continue;
        hit = true;
        int faceType = faceTypeOfFace[fi];
        auto take = [&](bool& have, int& slot) {
            if (!have) { have = true; slot = faceType; return true; }
            return slot == faceType;
        };
        if (side == FaceSide::Left || side == FaceSide::Both) {
            if (!take(haveLeft, left)) return {SampleKind::Conflict, -1, -1};
        }
        if (side == FaceSide::Right || side == FaceSide::Both) {
            if (!take(haveRight, right)) return {SampleKind::Conflict, -1, -1};
        }
    }
    if (!hit) return {SampleKind::Off, -1, -1};
    return {SampleKind::Ready, haveLeft ? left : -1, haveRight ? right : -1};
}

struct PolyCorner {
    int   vertex;
    int   edgeType;
    bool  isAtStart;
    Vec3d outward;
    Vec3d normal;
};

// Label every OBJ polyline segment with the face types on its two sides and
// intern that pair as an edge type. A segment may cross several coplanar
// faces; the pair has to stay the same on every piece. `corners` records
// each end so a polyline-only vertex can be given a type.
bool addPolylineEdgeTypes(
    const HalfEdgeMesh& mesh,
    const ObjMesh& obj,
    const TypeExtractionConfig& cfg,
    ExtractedTypes& out,
    std::unordered_map<EdgeTypeKey, int, EdgeTypeKeyHash>& edgeTypeMap,
    std::vector<PolyCorner>& corners,
    std::string* error
) {
    if (obj.polylines.empty()) return true;

    const double tol = cfg.positionEps;
    std::vector<FaceGeom> geoms;
    geoms.reserve(mesh.faces.size());
    for (int fi = 0; fi < (int)mesh.faces.size(); ++fi) {
        geoms.push_back(buildFaceGeom(mesh, obj, fi, tol));
    }

    auto fail = [&](int pi, int va, int vb, const char* why) {
        if (error) {
            std::ostringstream oss;
            oss << "polyline " << pi << " segment " << (va + 1)
                << " -> " << (vb + 1) << " " << why;
            *error = oss.str();
        }
        return false;
    };

    const int nVerts = (int)obj.vertices.size();
    for (int pi = 0; pi < (int)obj.polylines.size(); ++pi) {
        const auto& poly = obj.polylines[pi];
        for (int s = 0; s + 1 < (int)poly.vertexIndices.size(); ++s) {
            int va = poly.vertexIndices[s];
            int vb = poly.vertexIndices[s + 1];
            if (va < 0 || vb < 0 || va >= nVerts || vb >= nVerts) {
                if (error) {
                    std::ostringstream oss;
                    oss << "polyline " << pi << " references an out-of-range vertex";
                    *error = oss.str();
                }
                return false;
            }
            if (va == vb) continue;

            Vec3d A = atVertex(obj, va);
            Vec3d B = atVertex(obj, vb);
            Vec3d delta = sub3(B, A);
            double segLen = len3(delta);
            if (segLen <= 1e-15) continue;
            Vec3d dir{delta.x / segLen, delta.y / segLen, delta.z / segLen};

            if (!pointOnAnyFace(geoms, A, tol) || !pointOnAnyFace(geoms, B, tol)) {
                return fail(pi, va, vb, "does not lie on the mesh");
            }

            std::vector<int> candidates;
            std::vector<double> ts;
            ts.push_back(0.0);
            ts.push_back(1.0);
            for (int fi = 0; fi < (int)geoms.size(); ++fi) {
                const FaceGeom& g = geoms[fi];
                if (!g.valid) continue;
                if (std::abs(planeDist(g, A)) > g.planeTol) continue;
                if (std::abs(planeDist(g, B)) > g.planeTol) continue;
                candidates.push_back(fi);
                Vec2d a2 = projectFace(g, A);
                Vec2d b2 = projectFace(g, B);
                for (const auto& loop : g.loops) {
                    int n = (int)loop.size();
                    for (int i = 0; i < n; ++i) {
                        addSegmentCuts(a2, b2, loop[i], loop[(i + 1) % n], tol, ts);
                    }
                }
            }
            if (candidates.empty()) {
                return fail(pi, va, vb, "does not lie on the mesh");
            }

            double tTol = dmax(tol / segLen, 1e-9);
            std::sort(ts.begin(), ts.end());
            std::vector<double> cuts;
            for (double t : ts) {
                if (t < 0.0) t = 0.0;
                if (t > 1.0) t = 1.0;
                if (cuts.empty() || t > cuts.back() + tTol) cuts.push_back(t);
            }
            if (cuts.empty() || cuts.front() > 0.0) cuts.insert(cuts.begin(), 0.0);
            if (cuts.back() < 1.0) cuts.push_back(1.0);

            auto atT = [&](double t) {
                return Vec3d{A.x + delta.x * t, A.y + delta.y * t, A.z + delta.z * t};
            };

            bool haveRef = false;
            int refLeft = -1;
            int refRight = -1;
            bool sawSpan = false;
            for (size_t i = 0; i + 1 < cuts.size(); ++i) {
                double t0 = cuts[i];
                double t1 = cuts[i + 1];
                if ((t1 - t0) * segLen <= tol) continue;
                sawSpan = true;
                SampledSide sample = sampleSides(
                    obj, geoms, candidates, out.faceTypeOfFace,
                    A, B, atT(0.5 * (t0 + t1)), dir, tol);
                if (sample.kind == SampleKind::Conflict) {
                    return fail(pi, va, vb,
                                "has more than one face type on the same side");
                }
                if (sample.kind == SampleKind::Off) {
                    return fail(pi, va, vb, "does not lie on the mesh");
                }
                if (!haveRef) {
                    refLeft = sample.left;
                    refRight = sample.right;
                    haveRef = true;
                } else if (sample.left != refLeft || sample.right != refRight) {
                    return fail(pi, va, vb, "crosses more than one pair of face types");
                }
            }
            if (!sawSpan) {
                SampledSide sample = sampleSides(
                    obj, geoms, candidates, out.faceTypeOfFace,
                    A, B, atT(0.5), dir, tol);
                if (sample.kind != SampleKind::Ready) {
                    return fail(pi, va, vb, "does not lie on the mesh");
                }
                refLeft = sample.left;
                refRight = sample.right;
                haveRef = true;
            }
            if (!haveRef) {
                return fail(pi, va, vb, "does not lie on the mesh");
            }

            // A boundary mesh edge keeps its one face on the left of `dir`
            // and does not flip to the greater direction. Match that here so
            // a polyline traced along an open boundary reuses that edge type.
            bool flipped = refLeft < 0 && refRight >= 0;
            Vec3d useDir = dir;
            int left = refLeft;
            int right = refRight;
            if (flipped) {
                left = right;
                right = -1;
                useDir = {-dir.x, -dir.y, -dir.z};
            }
            InternedEdge interned = internEdgeType(
                out, edgeTypeMap, left, right, useDir, cfg.dirEps);
            bool aIsAtStart = flipped ? interned.reversed : !interned.reversed;
            Vec3d normal{0, 0, 1};
            Vec3d mid = atT(0.5);
            for (const auto& geom : geoms) {
                if (pointOnFace(geom, mid, tol)) {
                    normal = geom.normal;
                    break;
                }
            }
            corners.push_back({va, interned.index, aIsAtStart, dir, normal});
            corners.push_back({vb, interned.index, !aIsAtStart,
                               Vec3d{-dir.x, -dir.y, -dir.z}, normal});
        }
    }
    return true;
}

}  // namespace

bool extractTypes(const HalfEdgeMesh&         mesh,
                  const ObjMesh&              obj,
                  const TypeExtractionConfig& cfg,
                  ExtractedTypes&             out,
                  std::string*                error) {
    out = {};

    // --- Face types ------------------------------------------------
    out.faceTypeOfFace.resize(mesh.faces.size(), -1);
    std::unordered_map<FaceTypeKey, int, FaceTypeKeyHash> faceTypeMap;
    for (size_t fi = 0; fi < mesh.faces.size(); ++fi) {
        const auto& f = mesh.faces[fi];
        std::string mat;
        Vec3d color{1.0, 1.0, 1.0};
        if (f.materialId >= 0 && f.materialId < (int)obj.materialNames.size()) {
            mat = obj.materialNames[f.materialId];
            if (f.materialId < (int)obj.materialColors.size()) {
                const auto& c = obj.materialColors[f.materialId];
                color = {c.x, c.y, c.z};
            }
        }

        FaceTypeKey key{mat, f.volAbove, f.volBelow, q3(f.normal, cfg.normalEps)};
        auto it = faceTypeMap.find(key);
        int idx;
        if (it == faceTypeMap.end()) {
            idx = (int)out.faceTypes.size();
            out.faceTypes.push_back({mat, color, f.normal, f.volAbove, f.volBelow});
            faceTypeMap.emplace(std::move(key), idx);
        } else {
            idx = it->second;
        }
        out.faceTypeOfFace[fi] = idx;
    }

    // --- Edge types ------------------------------------------------
    out.edgeTypeOfEdge.resize(mesh.edges.size(), -1);
    // 1 when this edge's halfA direction is opposite the type's canonical dir.
    std::vector<char> edgeReversed(mesh.edges.size(), 0);
    std::unordered_map<EdgeTypeKey, int, EdgeTypeKeyHash> edgeTypeMap;
    for (size_t ei = 0; ei < mesh.edges.size(); ++ei) {
        if (isSlitEdge(mesh, (int)ei)) continue;
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

        InternedEdge interned = internEdgeType(
            out, edgeTypeMap, leftFaceType, rightFaceType, dir, cfg.dirEps);
        edgeReversed[ei] = interned.reversed ? 1 : 0;
        out.edgeTypeOfEdge[ei] = interned.index;
    }

    std::vector<PolyCorner> polyCorners;
    if (!addPolylineEdgeTypes(mesh, obj, cfg, out, edgeTypeMap, polyCorners, error)) {
        return false;
    }

    // --- Vertex types ------------------------------------------------
    // For each input vertex, walk the fan and collect (edgeType, isAtStart).
    // isAtStart = true iff this vertex is the origin of the edge type's
    // canonical direction. Canonicalize by lexicographically smallest cyclic
    // rotation, then dedupe.
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
                         "vertex %d: cannot enumerate fan (non-manifold)", v);
                *error = buf;
            }
            return false;
        }
        fan = dropSlitHalves(mesh, fan);

        // Build raw token sequence (edgeType, isAtStart). Skip slit halves so
        // a pinched n-gon does not add a fourth connection at the cut vertex.
        struct Slot { int edgeType; int isAtStart; };
        std::vector<Slot> rawSlots;
        std::vector<int> typedFan;
        rawSlots.reserve(fan.size());
        typedFan.reserve(fan.size());
        for (int he : fan) {
            int edgeIdx = mesh.halfEdges[he].edge;
            if (isSlitEdge(mesh, edgeIdx)) continue;
            int etIdx = out.edgeTypeOfEdge[edgeIdx];
            if (etIdx < 0) continue;
            // isAtStart = this vertex is the origin of the type's canonical
            // direction. halfA may point the other way; then the flag flips.
            bool atHalfAOrigin =
                (mesh.halfEdges[mesh.edges[edgeIdx].halfA].origin == v);
            bool isAtStart = edgeReversed[edgeIdx] ? !atHalfAOrigin : atHalfAOrigin;
            rawSlots.push_back({etIdx, isAtStart ? 1 : 0});
            typedFan.push_back(he);
        }
        if (rawSlots.empty()) continue;

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
        // Incoming boundary halves belong to another origin; do not overwrite
        // that vertex's slot.
        for (int k = 0; k < n; ++k) {
            int srcIdx = (best + k) % n;
            int he     = typedFan[srcIdx];
            if (mesh.halfEdges[he].origin == v) {
                out.halfEdgeSlotInVertex[he] = k;
            }
        }
    }

    // Vertices that only a polyline touches have no mesh fan. Order the
    // segments around the face normal, then use the same canonical rotation
    // as a mesh vertex.
    std::unordered_map<int, std::vector<PolyCorner>> byVertex;
    std::vector<int> polyVerts;
    for (const auto& corner : polyCorners) {
        auto& list = byVertex[corner.vertex];
        if (list.empty()) polyVerts.push_back(corner.vertex);
        list.push_back(corner);
    }
    std::sort(polyVerts.begin(), polyVerts.end());
    for (int v : polyVerts) {
        if (v < 0 || v >= (int)out.vertexTypeOfVertex.size()) continue;
        if (out.vertexTypeOfVertex[v] >= 0) continue;
        const auto& incident = byVertex[v];
        if (incident.empty()) continue;

        Vec3d normal = incident.front().normal;
        Vec3d axisU = tangentU(normal);
        Vec3d axisV = cross3(normal, axisU);
        double vlen = len3(axisV);
        if (vlen > 1e-15) {
            axisV.x /= vlen; axisV.y /= vlen; axisV.z /= vlen;
        }

        struct Slot { int edgeType; int isAtStart; double angle; };
        std::vector<Slot> rawSlots;
        rawSlots.reserve(incident.size());
        for (const auto& corner : incident) {
            double x = dot3(corner.outward, axisU);
            double y = dot3(corner.outward, axisV);
            rawSlots.push_back({corner.edgeType, corner.isAtStart ? 1 : 0, std::atan2(y, x)});
        }
        std::stable_sort(rawSlots.begin(), rawSlots.end(), [](const Slot& a, const Slot& b) {
            return a.angle < b.angle;
        });

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
    }

    return true;
}

}  // namespace mesh_extraction
