#include "pch.h"
#include "face_subdivision.h"
#include "face.h"
#include "face_group.h"
#include "half_edge.h"
#include "edge.h"
#include "vertex.h"
#include "../primitives/face_type.h"
#include "../third_party/earcut/earcut.h"
#include "../util/util.h"
#include "../geometry/intersector.h"
#include "../geometry/kd_tree.h"
#include "../settings.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <unordered_map>
#include <unordered_set>

struct FaceSubdivisionData {
    struct Key {
        int kind;
        int id;
        int index;

        bool operator==(const Key& other) const {
            return kind == other.kind && id == other.id && index == other.index;
        }
    };

    struct KeyHash {
        size_t operator()(const Key& key) const {
            size_t h = std::hash<int>{}(key.kind);
            h ^= std::hash<int>{}(key.id) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<int>{}(key.index) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };

    struct Sample {
        Vec3 base;
        Vec3 displaced;
        vector<int> faceIds;
    };

    struct EdgeInfo {
        int vertexA;
        int vertexB;
        int segments;
        vector<int> faceIds;
    };

    struct Tri {
        int a, b, c;
    };

    struct CachedFace {
        uint64_t signature = 0;
        Vec3 normal;
        double intensity = 0.0;
        double scale = 1.0;
        double decay = 0.0;
        double planeD = 0.0;
        Aabb aabb;
        vector<Vec3> positions;
        vector<int> edgeIds;
        vector<Key> boundaryKeys;
        vector<Key> holeKeys;
        vector<Key> interiorKeys;
        vector<Tri> triangles;
    };

    struct CachedRound {
        Vec3 start;
        Vec3 end;
        Vec3 dir;
        Vec3 bisector;
        Vec3 tangent1;
        Vec3 tangent2;
        int faceIdA = -1;
        int faceIdB = -1;
        double radius = 0.0;
        double centerDistance = 0.0;
        double influenceRadius = 0.0;
        Aabb aabb;
    };

    unordered_map<Key, Sample, KeyHash> samples;
    unordered_map<int, EdgeInfo> edges;
    unordered_map<int, CachedFace> faces;
    unordered_map<int, CachedRound> rounds;
    SampleKdTree tree;
    bool treeValid = false;
    bool fieldsApplied = false;
};

namespace {

using Key = FaceSubdivisionData::Key;
using Sample = FaceSubdivisionData::Sample;
using EdgeInfo = FaceSubdivisionData::EdgeInfo;
using Tri = FaceSubdivisionData::Tri;
using CachedFace = FaceSubdivisionData::CachedFace;
using CachedRound = FaceSubdivisionData::CachedRound;

struct BoundaryPoint {
    Key key;
    Vec2 uv;
};

constexpr int VERTEX_KEY = 0;
constexpr int EDGE_KEY = 1;
constexpr int INTERIOR_KEY = 2;

Key vertexKey(int vertexId) {
    return Key{VERTEX_KEY, vertexId, 0};
}

Key edgeKey(int edgeId, int index) {
    return Key{EDGE_KEY, edgeId, index};
}

Key interiorKey(int faceId, int index) {
    return Key{INTERIOR_KEY, faceId, index};
}

uint32_t hashBits(int a, int b, int c) {
    uint32_t h = static_cast<uint32_t>(getRandomSeed()) * 0x9E3779B9u;
    h ^= static_cast<uint32_t>(a) * 0x85EBCA6Bu;
    h ^= static_cast<uint32_t>(b) * 0xC2B2AE35u;
    h ^= static_cast<uint32_t>(c) * 0x27D4EB2Fu;
    h ^= h >> 16;
    h *= 0x7FEB352Du;
    h ^= h >> 15;
    h *= 0x846CA68Bu;
    h ^= h >> 16;
    return h;
}

double hashUnit(int a, int b, int c) {
    return (hashBits(a, b, c) >> 8) * (1.0 / 16777216.0);
}

double fade(double t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

double gradDot(uint32_t h, double x, double y, double z) {
    switch (h % 12u) {
    case 0: return x + y;
    case 1: return -x + y;
    case 2: return x - y;
    case 3: return -x - y;
    case 4: return x + z;
    case 5: return -x + z;
    case 6: return x - z;
    case 7: return -x - z;
    case 8: return y + z;
    case 9: return -y + z;
    case 10: return y - z;
    default: return -y - z;
    }
}

// Gradient noise. Value noise sat on a lattice and the old hash barely changed
// with Y or Z, so the surface looked like stripes along X.
double gradientNoise(const Vec3& p) {
    int x0 = static_cast<int>(std::floor(p.getX()));
    int y0 = static_cast<int>(std::floor(p.getY()));
    int z0 = static_cast<int>(std::floor(p.getZ()));
    double tx = fade(p.getX() - x0);
    double ty = fade(p.getY() - y0);
    double tz = fade(p.getZ() - z0);

    auto sample = [&](int x, int y, int z) {
        return gradDot(hashBits(x, y, z), p.getX() - x, p.getY() - y, p.getZ() - z);
    };
    double c000 = sample(x0, y0, z0);
    double c100 = sample(x0 + 1, y0, z0);
    double c010 = sample(x0, y0 + 1, z0);
    double c110 = sample(x0 + 1, y0 + 1, z0);
    double c001 = sample(x0, y0, z0 + 1);
    double c101 = sample(x0 + 1, y0, z0 + 1);
    double c011 = sample(x0, y0 + 1, z0 + 1);
    double c111 = sample(x0 + 1, y0 + 1, z0 + 1);

    auto mix = [](double a, double b, double t) {
        return a + (b - a) * t;
    };
    double x00 = mix(c000, c100, tx);
    double x10 = mix(c010, c110, tx);
    double x01 = mix(c001, c101, tx);
    double x11 = mix(c011, c111, tx);
    return mix(mix(x00, x10, ty), mix(x01, x11, ty), tz) * (1.0 / 0.75);
}

double fractalNoise(const Vec3& p, double scale) {
    if (scale < 1e-6) {
        scale = 1.0;
    }
    double frequency = 1.0 / scale;
    double amplitude = 1.0;
    double sum = 0.0;
    double weight = 0.0;
    for (int octave = 0; octave < FACE_NOISE_OCTAVES; octave++) {
        double shift = octave * 19.0;
        Vec3 scaled(
            p.getX() * frequency + shift,
            p.getY() * frequency + shift * 2.3,
            p.getZ() * frequency + shift * 4.7
        );
        sum += amplitude * gradientNoise(scaled);
        weight += amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    if (weight == 0.0) {
        return 0.0;
    }
    return sum / weight;
}

int segmentCount(double length, double spacing) {
    if (spacing <= 1e-8) {
        spacing = globalSettings["Face Grid Spacing"].get<double>();
    }
    int count = static_cast<int>(std::floor(length / spacing + 1e-9));
    return std::max(1, count);
}

double faceSpacing(const Face* face) {
    double spacing = face->getFaceType()->getGridSpacing();
    if (spacing <= 1e-8) {
        return globalSettings["Face Grid Spacing"].get<double>();
    }
    return spacing;
}

double edgeSpacing(Edge* edge) {
    double spacing = std::numeric_limits<double>::infinity();
    if (!edge) {
        return globalSettings["Face Grid Spacing"].get<double>();
    }
    for (HalfEdge* halfEdge : edge->getHalfEdges()) {
        if (!halfEdge) {
            continue;
        }
        Face* face = halfEdge->getFace();
        if (!face || face->isHole()) {
            continue;
        }
        spacing = std::min(spacing, faceSpacing(face));
    }
    if (!(spacing > 0.0) || spacing == std::numeric_limits<double>::infinity()) {
        return globalSettings["Face Grid Spacing"].get<double>();
    }
    return spacing;
}

Sample makeSample(const Vec3& position) {
    Sample sample;
    sample.base = position;
    sample.displaced = position;
    return sample;
}

void addFaceRef(vector<int>& faceIds, int faceId) {
    for (int id : faceIds) {
        if (id == faceId) {
            return;
        }
    }
    faceIds.push_back(faceId);
}

void removeFaceRef(vector<int>& faceIds, int faceId) {
    Util::remove(faceIds, faceId);
}

void ensureVertex(FaceSubdivisionData& data, Vertex* vertex) {
    if (!vertex) {
        return;
    }
    Key key = vertexKey(vertex->getId());
    if (data.samples.find(key) != data.samples.end()) {
        return;
    }
    data.samples.emplace(key, makeSample(vertex->getPosition()));
}

EdgeInfo& ensureEdge(FaceSubdivisionData& data, HalfEdge* halfEdge, int faceId) {
    Edge* edge = halfEdge->getEdge();
    int edgeId = edge->getId();
    auto found = data.edges.find(edgeId);
    if (found != data.edges.end()) {
        addFaceRef(found->second.faceIds, faceId);
        return found->second;
    }

    Vertex* start = halfEdge->getVertex();
    Vertex* end = halfEdge->next()->getVertex();
    Vertex* vertexA = start;
    Vertex* vertexB = end;
    if (start->getId() > end->getId()) {
        std::swap(vertexA, vertexB);
    }
    ensureVertex(data, vertexA);
    ensureVertex(data, vertexB);

    Vec3 from = vertexA->getPosition();
    Vec3 to = vertexB->getPosition();
    int segments = segmentCount((to - from).length(), edgeSpacing(edge));
    for (int i = 1; i < segments; i++) {
        double t = static_cast<double>(i) / static_cast<double>(segments);
        double delta = (hashUnit(edgeId, i, 3) - 0.5) * FACE_GRID_JITTER / static_cast<double>(segments);
        double lo = (static_cast<double>(i) - 0.45) / static_cast<double>(segments);
        double hi = (static_cast<double>(i) + 0.45) / static_cast<double>(segments);
        t = std::clamp(t + delta, lo, hi);
        data.samples.emplace(edgeKey(edgeId, i), makeSample(Vec3::lerp(from, to, t)));
    }

    EdgeInfo info{vertexA->getId(), vertexB->getId(), segments, {faceId}};
    return data.edges.emplace(edgeId, info).first->second;
}

void emitEdgePoints(
    const EdgeInfo& info,
    int edgeId,
    int startId,
    const std::function<void(const Key&)>& emit
) {
    if (startId == info.vertexA) {
        emit(vertexKey(info.vertexA));
        for (int i = 1; i < info.segments; i++) {
            emit(edgeKey(edgeId, i));
        }
        return;
    }
    if (startId == info.vertexB) {
        emit(vertexKey(info.vertexB));
        for (int i = info.segments - 1; i >= 1; i--) {
            emit(edgeKey(edgeId, i));
        }
        return;
    }
    emit(vertexKey(startId));
}

vector<BoundaryPoint> traceBoundary(
    FaceSubdivisionData& data,
    Face* face,
    const Vec3& u,
    const Vec3& v,
    int ownerId,
    bool create
) {
    vector<BoundaryPoint> boundary;
    auto pushPoint = [&](const Key& key) {
        auto found = data.samples.find(key);
        if (found == data.samples.end()) {
            return;
        }
        if (create) {
            addFaceRef(found->second.faceIds, ownerId);
        }
        const Vec3& base = found->second.base;
        if (!boundary.empty()) {
            auto previous = data.samples.find(boundary.back().key);
            if (previous != data.samples.end() && (base - previous->second.base).length2() < 1e-16) {
                return;
            }
        }
        boundary.push_back(BoundaryPoint{key, Vec2(u.dot(base), v.dot(base))});
    };

    for (HalfEdge* halfEdge : face->getHalfEdges()) {
        if (!halfEdge || !halfEdge->getVertex() || !halfEdge->getEdge()) {
            continue;
        }
        int edgeId = halfEdge->getEdge()->getId();
        int startId = halfEdge->getVertex()->getId();
        if (!create) {
            auto found = data.edges.find(edgeId);
            if (found == data.edges.end()) {
                pushPoint(vertexKey(startId));
                continue;
            }
            emitEdgePoints(found->second, edgeId, startId, pushPoint);
            continue;
        }
        HalfEdge* next = halfEdge->next();
        if (!next || !next->getVertex()) {
            ensureVertex(data, halfEdge->getVertex());
            pushPoint(vertexKey(startId));
            continue;
        }
        EdgeInfo& info = ensureEdge(data, halfEdge, ownerId);
        emitEdgePoints(info, edgeId, startId, pushPoint);
    }

    if (boundary.size() >= 2) {
        auto first = data.samples.find(boundary.front().key);
        auto last = data.samples.find(boundary.back().key);
        if (first != data.samples.end() && last != data.samples.end()
            && (first->second.base - last->second.base).length2() < 1e-16) {
            boundary.pop_back();
        }
    }
    return boundary;
}

vector<Vec2> boundaryUv(const vector<BoundaryPoint>& boundary) {
    vector<Vec2> uv;
    uv.reserve(boundary.size());
    for (const BoundaryPoint& point : boundary) {
        uv.push_back(point.uv);
    }
    return uv;
}

bool boundaryCrossesCell(const vector<Vec2>& polygon, const Vec2 corners[4]) {
    double minX = corners[0].x;
    double maxX = corners[2].x;
    double minY = corners[0].y;
    double maxY = corners[2].y;
    size_t n = polygon.size();
    for (size_t i = 0; i < n; i++) {
        const Vec2& a = polygon[i];
        const Vec2& b = polygon[(i + 1) % n];
        if (a.x > minX && a.x < maxX && a.y > minY && a.y < maxY) {
            return true;
        }
        if (std::max(a.x, b.x) < minX || std::min(a.x, b.x) > maxX) {
            continue;
        }
        if (std::max(a.y, b.y) < minY || std::min(a.y, b.y) > maxY) {
            continue;
        }
        for (int edge = 0; edge < 4; edge++) {
            if (Intersector::intersect(a, b, corners[edge], corners[(edge + 1) % 4])) {
                return true;
            }
        }
    }
    return false;
}

bool pointInRing(const Vec2& point, const vector<Vec2>& ring) {
    double x = point.x;
    double y = point.y;
    int count = 0;
    size_t n = ring.size();
    for (size_t i = 0; i < n; i++) {
        double y1 = ring[i].y;
        double y2 = ring[(i + 1) % n].y;
        double x1 = ring[i].x;
        double x2 = ring[(i + 1) % n].x;
        if (!((y1 > y && y2 < y) || (y1 < y && y2 > y))) {
            continue;
        }
        double s = (y - y1) / (y2 - y1);
        double xCross = x1 + (x2 - x1) * s;
        if (x < xCross) {
            count++;
        }
    }
    return (count % 2) == 1;
}

// True when any vertex or the centroid of `hole` lies inside `container`.
bool ringContainsHole(const vector<Vec2>& container, const vector<Vec2>& hole) {
    if (hole.empty() || container.size() < 3) {
        return false;
    }
    Vec2 centroid(0, 0);
    int inside = 0;
    for (const Vec2& point : hole) {
        centroid = Vec2(centroid.x + point.x, centroid.y + point.y);
        if (pointInRing(point, container)) {
            inside++;
        }
    }
    if (inside > 0) {
        return true;
    }
    double scale = 1.0 / static_cast<double>(hole.size());
    return pointInRing(Vec2(centroid.x * scale, centroid.y * scale), container);
}

bool ringsCross(const vector<Vec2>& a, const vector<Vec2>& b) {
    if (a.size() < 2 || b.size() < 2) {
        return false;
    }
    for (size_t i = 0; i < a.size(); i++) {
        const Vec2& a0 = a[i];
        const Vec2& a1 = a[(i + 1) % a.size()];
        for (size_t j = 0; j < b.size(); j++) {
            if (Intersector::intersect(a0, a1, b[j], b[(j + 1) % b.size()])) {
                return true;
            }
        }
    }
    return false;
}

// Outer ring plus hole rings. An empty outerLocals list means indices 0..n-1.
// The outer ring is wound counterclockwise. Hole rings are wound the other way.
void earcutSolid(
    const vector<Vec2>& outer,
    const vector<int>& outerLocals,
    const vector<vector<Vec2>>& holeRings,
    const vector<vector<int>>& holeLocals,
    vector<Tri>& triangles
) {
    if (outer.size() < 3) {
        return;
    }
    using Point = std::array<double, 2>;
    vector<Vec2> outerRing = outer;
    vector<int> outerIndex;
    if (outerLocals.size() == outer.size()) {
        outerIndex = outerLocals;
    } else {
        outerIndex.resize(outer.size());
        for (int i = 0; i < static_cast<int>(outer.size()); i++) {
            outerIndex[i] = i;
        }
    }
    if (Util::signedArea(outerRing) < 0.0) {
        std::reverse(outerRing.begin(), outerRing.end());
        std::reverse(outerIndex.begin(), outerIndex.end());
    }
    double outerArea = Util::signedArea(outerRing);
    vector<vector<Point>> rings(1);
    vector<int> earcutToLocal;
    rings[0].reserve(outerRing.size());
    earcutToLocal.reserve(outerRing.size());
    for (int i = 0; i < static_cast<int>(outerRing.size()); i++) {
        rings[0].push_back({outerRing[i].x, outerRing[i].y});
        earcutToLocal.push_back(outerIndex[i]);
    }
    for (size_t h = 0; h < holeRings.size(); h++) {
        vector<Vec2> ring = holeRings[h];
        vector<int> locals = h < holeLocals.size() ? holeLocals[h] : vector<int>{};
        if (ring.size() < 3 || ring.size() != locals.size()) {
            continue;
        }
        if (Util::signedArea(ring) * outerArea > 0.0) {
            std::reverse(ring.begin(), ring.end());
            std::reverse(locals.begin(), locals.end());
        }
        vector<Point> points;
        points.reserve(ring.size());
        for (size_t i = 0; i < ring.size(); i++) {
            points.push_back({ring[i].x, ring[i].y});
            earcutToLocal.push_back(locals[i]);
        }
        rings.push_back(std::move(points));
    }
    auto indices = mapbox::earcut(rings);
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        int i0 = static_cast<int>(indices[i]);
        int i1 = static_cast<int>(indices[i + 1]);
        int i2 = static_cast<int>(indices[i + 2]);
        if (i0 < 0 || i1 < 0 || i2 < 0
            || i0 >= static_cast<int>(earcutToLocal.size())
            || i1 >= static_cast<int>(earcutToLocal.size())
            || i2 >= static_cast<int>(earcutToLocal.size())) {
            continue;
        }
        int a = earcutToLocal[i0];
        int b = earcutToLocal[i1];
        int c = earcutToLocal[i2];
        if (a == b || b == c || c == a) {
            continue;
        }
        triangles.push_back(Tri{a, b, c});
    }
}

// Walk the boundary and the grid perimeter together. Each step advances the
// loop whose next vertex is closer, so the strip is a band of short triangles.
void stitchBoundary(
    const vector<Vec2>& polygon,
    const vector<Vec2>& interiorUv,
    int boundaryCount,
    const vector<int>& hole,
    vector<Tri>& triangles
) {
    int boundarySize = boundaryCount;
    int holeSize = static_cast<int>(hole.size());
    if (boundarySize < 3 || holeSize < 3) {
        return;
    }

    auto uvOf = [&](int local) {
        if (local < boundaryCount) {
            return polygon[local];
        }
        return interiorUv[local - boundaryCount];
    };

    int bestBoundary = 0;
    int bestHole = 0;
    double bestDistance = std::numeric_limits<double>::infinity();
    for (int boundaryIndex = 0; boundaryIndex < boundarySize; boundaryIndex++) {
        for (int holeIndex = 0; holeIndex < holeSize; holeIndex++) {
            double distance = (uvOf(boundaryIndex) - uvOf(hole[holeIndex])).length2();
            if (distance < bestDistance) {
                bestDistance = distance;
                bestBoundary = boundaryIndex;
                bestHole = holeIndex;
            }
        }
    }

    int boundaryIndex = bestBoundary;
    int holeIndex = bestHole;
    int boundarySteps = 0;
    int holeSteps = 0;
    while (boundarySteps < boundarySize || holeSteps < holeSize) {
        int boundaryNext = (boundaryIndex + 1) % boundarySize;
        int holeNext = (holeIndex + 1) % holeSize;
        bool advanceBoundary = boundarySteps < boundarySize;
        bool advanceHole = holeSteps < holeSize;
        if (advanceBoundary && advanceHole) {
            double boundaryAdvance = (uvOf(boundaryNext) - uvOf(hole[holeIndex])).length2();
            double holeAdvance = (uvOf(boundaryIndex) - uvOf(hole[holeNext])).length2();
            if (boundaryAdvance <= holeAdvance) {
                advanceHole = false;
            } else {
                advanceBoundary = false;
            }
        }
        if (advanceBoundary) {
            triangles.push_back(Tri{boundaryIndex, boundaryNext, hole[holeIndex]});
            boundaryIndex = boundaryNext;
            boundarySteps++;
        } else {
            triangles.push_back(Tri{boundaryIndex, hole[holeNext], hole[holeIndex]});
            holeIndex = holeNext;
            holeSteps++;
        }
    }
}

// Interior samples are connected as grid quads. The strip between that grid
// and the face boundary is stitched to the perimeter so it stays a band.
void buildGridMesh(
    Face* face,
    FaceSubdivisionData& data,
    CachedFace& cached,
    const vector<Vec2>& polygon,
    const Vec3& axisU,
    const Vec3& axisV,
    const Vec3& normal,
    double plane
) {
    int boundaryCount = static_cast<int>(polygon.size());
    if (boundaryCount < 3) {
        return;
    }

    // Hole faces in this group are contours cut out of this face. Their
    // samples belong to this face so the cutout stays in the mesh.
    struct FaceHole {
        vector<Vec2> uv;
        vector<int> local;
        bool handled = false;
    };
    vector<FaceHole> faceHoles;
    vector<Vec2> holeUv;
    if (!face->isHole()) {
        FaceGroup* group = face->getGroup();
        if (group) {
            for (Face* other : group->getFaces()) {
                if (!other || other == face || !other->isHole()) {
                    continue;
                }
                vector<BoundaryPoint> loop = traceBoundary(data, other, axisU, axisV, face->getId(), true);
                if (loop.size() < 3) {
                    continue;
                }
                FaceHole faceHole;
                for (const BoundaryPoint& point : loop) {
                    faceHole.local.push_back(boundaryCount + static_cast<int>(holeUv.size()));
                    faceHole.uv.push_back(point.uv);
                    holeUv.push_back(point.uv);
                    cached.holeKeys.push_back(point.key);
                }
                faceHoles.push_back(std::move(faceHole));
                for (HalfEdge* halfEdge : other->getHalfEdges()) {
                    if (halfEdge && halfEdge->getEdge()) {
                        cached.edgeIds.push_back(halfEdge->getEdge()->getId());
                    }
                }
            }
        }
    }
    int holeCount = static_cast<int>(holeUv.size());
    auto inHole = [&](const Vec2& point) {
        for (const FaceHole& faceHole : faceHoles) {
            if (pointInRing(point, faceHole.uv)) {
                return true;
            }
        }
        return false;
    };

    double spacing = faceSpacing(face);
    double uMin = std::numeric_limits<double>::infinity();
    double uMax = -uMin;
    double vMin = uMin;
    double vMax = uMax;
    for (const Vec2& point : polygon) {
        uMin = std::min(uMin, point.x);
        uMax = std::max(uMax, point.x);
        vMin = std::min(vMin, point.y);
        vMax = std::max(vMax, point.y);
    }

    vector<double> uCoordinates = Util::evenlySpaced(uMin, uMax, spacing);
    vector<double> vCoordinates = Util::evenlySpaced(vMin, vMax, spacing);
    int cols = static_cast<int>(uCoordinates.size());
    int rows = static_cast<int>(vCoordinates.size());
    if (cols < 2 || rows < 2) {
        vector<vector<Vec2>> holeRings;
        vector<vector<int>> holeLocals;
        for (const FaceHole& faceHole : faceHoles) {
            holeRings.push_back(faceHole.uv);
            holeLocals.push_back(faceHole.local);
        }
        earcutSolid(polygon, {}, holeRings, holeLocals, cached.triangles);
        return;
    }

    auto slot = [cols](int iu, int iv) {
        return iu + iv * cols;
    };
    vector<char> inside(cols * rows, 0);
    for (int iv = 0; iv < rows; iv++) {
        for (int iu = 0; iu < cols; iu++) {
            Vec2 sample(uCoordinates[iu], vCoordinates[iv]);
            Vec3 world = axisU * sample.x + axisV * sample.y + normal * plane;
            if (face->containsPoint(world) && !inHole(sample)) {
                inside[slot(iu, iv)] = 1;
            }
        }
    }

    vector<pair<int, int>> cells;
    for (int iv = 0; iv + 1 < rows; iv++) {
        for (int iu = 0; iu + 1 < cols; iu++) {
            if (!inside[slot(iu, iv)] || !inside[slot(iu + 1, iv)]
                || !inside[slot(iu + 1, iv + 1)] || !inside[slot(iu, iv + 1)]) {
                continue;
            }
            Vec2 corners[4] = {
                Vec2(uCoordinates[iu], vCoordinates[iv]),
                Vec2(uCoordinates[iu + 1], vCoordinates[iv]),
                Vec2(uCoordinates[iu + 1], vCoordinates[iv + 1]),
                Vec2(uCoordinates[iu], vCoordinates[iv + 1])
            };
            if (boundaryCrossesCell(polygon, corners)) {
                continue;
            }
            Vec2 center(
                (corners[0].x + corners[2].x) * 0.5,
                (corners[0].y + corners[2].y) * 0.5
            );
            if (inHole(center)) {
                continue;
            }
            bool holeCrosses = false;
            for (const FaceHole& faceHole : faceHoles) {
                if (boundaryCrossesCell(faceHole.uv, corners)) {
                    holeCrosses = true;
                    break;
                }
            }
            if (holeCrosses) {
                continue;
            }
            cells.push_back({iu, iv});
        }
    }

    vector<int> gridIndex(cols * rows, -1);
    vector<Vec2> interiorUv;
    auto addGridPoint = [&](int iu, int iv) {
        int index = slot(iu, iv);
        if (gridIndex[index] >= 0) {
            return gridIndex[index];
        }
        Vec2 point(uCoordinates[iu], vCoordinates[iv]);
        double ju = (hashUnit(face->getId(), iu, iv) - 0.5) * FACE_GRID_JITTER * spacing;
        double jv = (hashUnit(face->getId(), iv, iu + 17) - 0.5) * FACE_GRID_JITTER * spacing;
        Vec2 jittered(point.x + ju, point.y + jv);
        Vec3 jitteredWorld = axisU * jittered.x + axisV * jittered.y + normal * plane;
        Vec2 placed = (face->containsPoint(jitteredWorld) && !inHole(jittered)) ? jittered : point;
        Vec3 base = axisU * placed.x + axisV * placed.y + normal * plane;
        int local = boundaryCount + holeCount + static_cast<int>(cached.interiorKeys.size());
        Key key = interiorKey(face->getId(), static_cast<int>(cached.interiorKeys.size()));
        Sample sample = makeSample(base);
        addFaceRef(sample.faceIds, face->getId());
        data.samples.emplace(key, sample);
        cached.interiorKeys.push_back(key);
        interiorUv.push_back(point);
        gridIndex[index] = local;
        return local;
    };

    for (const auto& cell : cells) {
        int iu = cell.first;
        int iv = cell.second;
        int i00 = addGridPoint(iu, iv);
        int i10 = addGridPoint(iu + 1, iv);
        int i11 = addGridPoint(iu + 1, iv + 1);
        int i01 = addGridPoint(iu, iv + 1);
        cached.triangles.push_back(Tri{i00, i10, i11});
        cached.triangles.push_back(Tri{i00, i11, i01});
    }

    map<pair<int, int>, int> perimeter;
    auto addDirected = [&](int from, int to) {
        auto reverse = perimeter.find({to, from});
        if (reverse != perimeter.end()) {
            perimeter.erase(reverse);
        } else {
            perimeter[{from, to}] = 1;
        }
    };
    for (const auto& cell : cells) {
        int iu = cell.first;
        int iv = cell.second;
        int i00 = gridIndex[slot(iu, iv)];
        int i10 = gridIndex[slot(iu + 1, iv)];
        int i11 = gridIndex[slot(iu + 1, iv + 1)];
        int i01 = gridIndex[slot(iu, iv + 1)];
        addDirected(i00, i10);
        addDirected(i10, i11);
        addDirected(i11, i01);
        addDirected(i01, i00);
    }

    auto uvAt = [&](int local) -> Vec2 {
        if (local < boundaryCount) {
            return polygon[local];
        }
        if (local < boundaryCount + holeCount) {
            return holeUv[local - boundaryCount];
        }
        return interiorUv[local - boundaryCount - holeCount];
    };
    map<int, vector<int>> outgoing;
    for (const auto& edge : perimeter) {
        outgoing[edge.first.first].push_back(edge.first.second);
    }
    map<pair<int, int>, int> unused = perimeter;
    vector<vector<int>> holes;
    while (!unused.empty()) {
        int start = unused.begin()->first.first;
        int to = unused.begin()->first.second;
        vector<int> loop;
        loop.push_back(start);
        int from = start;
        bool closed = false;
        int guard = 0;
        int limit = static_cast<int>(perimeter.size()) + 2;
        while (guard++ < limit) {
            auto edge = unused.find({from, to});
            if (edge == unused.end()) {
                break;
            }
            unused.erase(edge);
            loop.push_back(to);
            if (to == start) {
                closed = true;
                break;
            }
            int next = -1;
            auto outs = outgoing.find(to);
            if (outs != outgoing.end()) {
                // Solid sits to the left of each directed edge. At a vertex
                // where two openings meet, take the sharpest right turn so
                // each hole keeps its own loop.
                double bestTurn = std::numeric_limits<double>::infinity();
                Vec2 back = uvAt(from) - uvAt(to);
                for (int candidate : outs->second) {
                    if (!unused.count({to, candidate})) {
                        continue;
                    }
                    Vec2 out = uvAt(candidate) - uvAt(to);
                    double cross = back.x * out.y - back.y * out.x;
                    double dot = back.x * out.x + back.y * out.y;
                    double angle = std::atan2(cross, dot);
                    if (angle <= 1e-9) {
                        angle += 6.28318530717958647692;
                    }
                    if (angle < bestTurn) {
                        bestTurn = angle;
                        next = candidate;
                    }
                }
            }
            if (next < 0) {
                break;
            }
            from = to;
            to = next;
        }
        if (closed && loop.size() >= 4) {
            loop.pop_back();
            holes.push_back(std::move(loop));
        }
    }

    auto loopArea = [&](const vector<int>& loop) {
        vector<Vec2> ring;
        ring.reserve(loop.size());
        for (int index : loop) {
            ring.push_back(interiorUv[index - boundaryCount]);
        }
        return Util::signedArea(ring);
    };
    double outerArea = Util::signedArea(polygon);
    if (!faceHoles.empty()) {
        auto ringFrom = [&](const vector<int>& loop) {
            vector<Vec2> ring;
            ring.reserve(loop.size());
            for (int index : loop) {
                ring.push_back(uvAt(index));
            }
            return ring;
        };
        auto cutWholeFace = [&]() {
            cached.triangles.clear();
            vector<vector<Vec2>> holeRings;
            vector<vector<int>> holeLocals;
            for (const FaceHole& faceHole : faceHoles) {
                holeRings.push_back(faceHole.uv);
                holeLocals.push_back(faceHole.local);
            }
            earcutSolid(polygon, {}, holeRings, holeLocals, cached.triangles);
        };

        vector<vector<int>> outerLoops;
        vector<vector<int>> innerLoops;
        for (vector<int>& loop : holes) {
            if (Util::signedArea(ringFrom(loop)) >= 0.0) {
                outerLoops.push_back(std::move(loop));
            } else {
                innerLoops.push_back(std::move(loop));
            }
        }

        // Several holes can share one grid opening when they sit closer
        // together than the grid. Earcut that opening with every hole it holds.
        vector<vector<int>> grouped(innerLoops.size());
        for (int h = 0; h < static_cast<int>(faceHoles.size()); h++) {
            int best = -1;
            double bestArea = std::numeric_limits<double>::infinity();
            for (int i = 0; i < static_cast<int>(innerLoops.size()); i++) {
                vector<Vec2> cavity = ringFrom(innerLoops[i]);
                if (!ringContainsHole(cavity, faceHoles[h].uv)) {
                    continue;
                }
                double area = std::abs(Util::signedArea(cavity));
                if (area < bestArea) {
                    bestArea = area;
                    best = i;
                }
            }
            if (best >= 0) {
                grouped[best].push_back(h);
                faceHoles[h].handled = true;
            }
        }

        bool nested = false;
        for (const FaceHole& faceHole : faceHoles) {
            if (faceHole.handled) {
                continue;
            }
            for (const vector<int>& loop : outerLoops) {
                vector<Vec2> ring = ringFrom(loop);
                if (ringContainsHole(ring, faceHole.uv) || ringsCross(faceHole.uv, ring)) {
                    nested = true;
                    break;
                }
            }
            if (nested) {
                break;
            }
        }
        if (nested) {
            cutWholeFace();
            return;
        }

        bool earcutFailed = false;
        for (int i = 0; i < static_cast<int>(innerLoops.size()); i++) {
            if (grouped[i].empty()) {
                continue;
            }
            vector<vector<Vec2>> holeRings;
            vector<vector<int>> holeLocals;
            for (int h : grouped[i]) {
                holeRings.push_back(faceHoles[h].uv);
                holeLocals.push_back(faceHoles[h].local);
            }
            size_t before = cached.triangles.size();
            earcutSolid(ringFrom(innerLoops[i]), innerLoops[i], holeRings, holeLocals, cached.triangles);
            if (cached.triangles.size() == before) {
                earcutFailed = true;
            }
        }
        if (earcutFailed) {
            cutWholeFace();
            return;
        }

        vector<vector<Vec2>> holeRings;
        vector<vector<int>> holeLocals;
        for (const vector<int>& loop : outerLoops) {
            holeRings.push_back(ringFrom(loop));
            holeLocals.push_back(loop);
        }
        for (const FaceHole& faceHole : faceHoles) {
            if (faceHole.handled) {
                continue;
            }
            holeRings.push_back(faceHole.uv);
            holeLocals.push_back(faceHole.local);
        }
        if (!holeRings.empty()) {
            size_t before = cached.triangles.size();
            earcutSolid(polygon, {}, holeRings, holeLocals, cached.triangles);
            if (cached.triangles.size() == before) {
                cutWholeFace();
            }
        }
        return;
    }
    if (holes.size() == 1) {
        vector<int>& hole = holes[0];
        if (loopArea(hole) * outerArea < 0.0) {
            std::reverse(hole.begin(), hole.end());
        }
        stitchBoundary(polygon, interiorUv, boundaryCount, hole, cached.triangles);
        return;
    }

    if (!cells.empty() && holes.empty()) {
        return;
    }

    using Point = std::array<double, 2>;
    vector<vector<Point>> rings(1);
    vector<int> earcutToLocal;
    rings[0].reserve(boundaryCount);
    earcutToLocal.reserve(boundaryCount);
    for (int i = 0; i < boundaryCount; i++) {
        rings[0].push_back({polygon[i].x, polygon[i].y});
        earcutToLocal.push_back(i);
    }
    for (vector<int>& hole : holes) {
        if (loopArea(hole) * outerArea > 0.0) {
            std::reverse(hole.begin(), hole.end());
        }
        vector<Point> ring;
        ring.reserve(hole.size());
        for (int local : hole) {
            Vec2 point = interiorUv[local - boundaryCount];
            ring.push_back({point.x, point.y});
            earcutToLocal.push_back(local);
        }
        rings.push_back(std::move(ring));
    }

    auto indices = mapbox::earcut(rings);
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        int i0 = static_cast<int>(indices[i]);
        int i1 = static_cast<int>(indices[i + 1]);
        int i2 = static_cast<int>(indices[i + 2]);
        if (i0 < 0 || i1 < 0 || i2 < 0
            || i0 >= static_cast<int>(earcutToLocal.size())
            || i1 >= static_cast<int>(earcutToLocal.size())
            || i2 >= static_cast<int>(earcutToLocal.size())) {
            continue;
        }
        cached.triangles.push_back(Tri{earcutToLocal[i0], earcutToLocal[i1], earcutToLocal[i2]});
    }
}

uint64_t faceSignature(Face* face) {
    uint64_t h = 1469598103934665603ull;
    auto mix = [&](uint64_t value) {
        h ^= value;
        h *= 1099511628211ull;
    };
    auto bits = [](double value) {
        uint64_t packed = 0;
        static_assert(sizeof(double) == sizeof(uint64_t), "double size");
        std::memcpy(&packed, &value, sizeof(packed));
        return packed;
    };
    auto mixLoop = [&](Face* loopFace) {
        if (!loopFace) {
            return;
        }
        for (HalfEdge* halfEdge : loopFace->getHalfEdges()) {
            if (!halfEdge || !halfEdge->getVertex()) {
                continue;
            }
            mix(static_cast<uint64_t>(static_cast<uint32_t>(halfEdge->getId())));
            mix(static_cast<uint64_t>(static_cast<uint32_t>(halfEdge->getVertex()->getId())));
            Vec3 position = halfEdge->getPosition();
            mix(bits(position.getX()));
            mix(bits(position.getY()));
            mix(bits(position.getZ()));
        }
    };
    mixLoop(face);
    if (!face->isHole()) {
        FaceGroup* group = face->getGroup();
        if (group) {
            for (Face* other : group->getFaces()) {
                if (other && other != face && other->isHole()) {
                    mix(1);
                    mixLoop(other);
                }
            }
        }
    }
    return h;
}

void fillFaceField(CachedFace& cached, Face* face) {
    FaceType* faceType = face->getFaceType();
    cached.normal = faceType->getNormal();
    cached.intensity = faceType->getNoiseIntensity();
    cached.scale = faceType->getNoiseScale();
    cached.decay = faceType->getNoiseDecay();
    cached.positions = face->getPositions();
    cached.aabb = Aabb();
    if (!cached.positions.empty()) {
        cached.aabb = Aabb(cached.positions[0]);
        cached.planeD = cached.normal.dot(cached.positions[0]);
        for (size_t i = 1; i < cached.positions.size(); i++) {
            cached.aabb.expand(cached.positions[i]);
        }
    }
}

void addFaceGeometry(FaceSubdivisionData& data, Face* face) {
    CachedFace cached;
    cached.signature = faceSignature(face);
    fillFaceField(cached, face);

    const Vec3& u = face->getFaceType()->getU();
    const Vec3& v = face->getFaceType()->getV();
    vector<BoundaryPoint> boundary = traceBoundary(data, face, u, v, face->getId(), true);
    for (const BoundaryPoint& point : boundary) {
        cached.boundaryKeys.push_back(point.key);
    }
    for (HalfEdge* halfEdge : face->getHalfEdges()) {
        if (halfEdge && halfEdge->getEdge()) {
            cached.edgeIds.push_back(halfEdge->getEdge()->getId());
        }
    }

    if (boundary.size() >= 3 && !cached.positions.empty()) {
        vector<Vec2> polygon = boundaryUv(boundary);
        buildGridMesh(face, data, cached, polygon, u, v, cached.normal, cached.planeD);
    }

    data.faces[face->getId()] = std::move(cached);
    data.treeValid = false;
}

void dropFaceGeometry(FaceSubdivisionData& data, int faceId) {
    auto found = data.faces.find(faceId);
    if (found == data.faces.end()) {
        return;
    }
    CachedFace& cached = found->second;
    for (const Key& key : cached.interiorKeys) {
        data.samples.erase(key);
    }
    auto dropShared = [&](const vector<Key>& keys) {
        for (const Key& key : keys) {
            auto sample = data.samples.find(key);
            if (sample == data.samples.end()) {
                continue;
            }
            removeFaceRef(sample->second.faceIds, faceId);
            if (sample->second.faceIds.empty()) {
                data.samples.erase(sample);
            }
        }
    };
    dropShared(cached.boundaryKeys);
    dropShared(cached.holeKeys);
    for (int edgeId : cached.edgeIds) {
        auto edge = data.edges.find(edgeId);
        if (edge == data.edges.end()) {
            continue;
        }
        removeFaceRef(edge->second.faceIds, faceId);
        if (edge->second.faceIds.empty()) {
            for (int i = 1; i < edge->second.segments; i++) {
                data.samples.erase(edgeKey(edgeId, i));
            }
            data.edges.erase(edge);
        }
    }
    data.faces.erase(found);
    data.treeValid = false;
}

void rebuildTree(FaceSubdivisionData& data) {
    vector<SampleKdTree::Point> points;
    points.reserve(data.samples.size());
    for (auto& entry : data.samples) {
        points.push_back({entry.second.base, &entry.second});
    }
    data.tree.build(points);
    data.treeValid = true;
}

void resetDisplacements(FaceSubdivisionData& data) {
    for (auto& entry : data.samples) {
        entry.second.displaced = entry.second.base;
    }
}

void applyFace(
    FaceSubdivisionData& data,
    const CachedFace& face,
    double sign,
    const unordered_set<Sample*>* filter
) {
    if (std::abs(face.intensity) <= FACE_NOISE_EPSILON && face.decay > 1e-12) {
        return;
    }
    if (!data.treeValid) {
        rebuildTree(data);
    }

    SampleKdTree::ProximityQuery query;
    query.bounds = face.aabb;
    query.planeNormal = face.normal;
    query.planeD = face.planeD;
    query.decay = face.decay;
    query.intensity = face.intensity;
    query.epsilon = FACE_NOISE_EPSILON;

    data.tree.visit(query, [&](void* payload) {
        Sample* sample = static_cast<Sample*>(payload);
        if (filter && filter->find(sample) == filter->end()) {
            return;
        }
        double distance = Face::distanceToPolygon(sample->base, face.normal, face.positions);
        double weight = 0.0;
        if (face.decay <= 1e-12) {
            weight = distance <= 1e-8 ? 1.0 : 0.0;
        } else {
            weight = std::exp(-face.decay * distance);
        }
        double amount = face.intensity * fractalNoise(sample->base, face.scale) * weight;
        if (std::abs(amount) < FACE_NOISE_EPSILON) {
            return;
        }
        sample->displaced += face.normal * (sign * amount);
    });
}

Vec3 closestOnSegment(const Vec3& a, const Vec3& b, const Vec3& point) {
    Vec3 ab = b - a;
    double ab2 = ab.length2();
    if (ab2 < 1e-20) {
        return a;
    }
    double t = std::clamp((point - a).dot(ab) / ab2, 0.0, 1.0);
    return a + ab * t;
}

Vec3 rejectAxis(const Vec3& v, const Vec3& axis) {
    return v - axis * axis.dot(v);
}

double angleAround(const Vec3& v, const Vec3& xAxis, const Vec3& yAxis) {
    return std::atan2(v.dot(yAxis), v.dot(xAxis));
}

bool angleInArc(double angle, double start, double end, double include) {
    const double twoPi = 6.28318530717958647692;
    auto ccw = [twoPi](double from, double to, double x) {
        double span = std::fmod(to - from + twoPi, twoPi);
        double delta = std::fmod(x - from + twoPi, twoPi);
        return delta <= span + 1e-9;
    };
    if (ccw(start, end, include)) {
        return ccw(start, end, angle);
    }
    return ccw(end, start, angle);
}

Vec3 intoFaceFromEdge(Face* face, const Vec3& start, const Vec3& end, const Vec3& dir) {
    Vec3 n = face->getFaceType()->getNormal();
    Vec3 side = dir.cross(n);
    if (side.length2() < 1e-20) {
        return Vec3(0, 0, 0);
    }
    side.normalize();
    Vec3 mid = Vec3::lerp(start, end, 0.5);
    double spacing = face->getFaceType()->getGridSpacing();
    if (spacing <= 1e-8) {
        spacing = 1.0;
    }
    Vec3 probe = mid + side * (0.25 * spacing);
    if (face->containsPoint(probe)) {
        return side;
    }
    return side * -1.0;
}

bool fillRound(
    CachedRound& round,
    Edge* edge,
    const unordered_map<int, Face*>& live
) {
    if (!edge || !edge->getEdgeType()) {
        return false;
    }
    double radius = edge->getEdgeType()->getRoundRadius();
    if (radius <= 1e-8) {
        return false;
    }

    vector<Face*> adjacent;
    for (HalfEdge* halfEdge : edge->getHalfEdges()) {
        if (!halfEdge) {
            continue;
        }
        Face* face = halfEdge->getFace();
        if (!face || face->isHole() || live.find(face->getId()) == live.end()) {
            continue;
        }
        bool seen = false;
        for (Face* existing : adjacent) {
            if (existing == face) {
                seen = true;
                break;
            }
        }
        if (!seen) {
            adjacent.push_back(face);
        }
    }
    if (adjacent.size() != 2) {
        return false;
    }

    vector<HalfEdge*> halfEdges = edge->getHalfEdges();
    if (halfEdges.empty() || !halfEdges[0] || !halfEdges[0]->next()) {
        return false;
    }
    Vec3 start = halfEdges[0]->getPosition();
    Vec3 end = halfEdges[0]->next()->getPosition();
    Vec3 dir = end - start;
    if (dir.length2() < 1e-20) {
        return false;
    }
    dir.normalize();

    Vec3 n1 = adjacent[0]->getFaceType()->getNormal();
    Vec3 n2 = adjacent[1]->getFaceType()->getNormal();
    n1 = rejectAxis(n1, dir);
    n2 = rejectAxis(n2, dir);
    if (n1.length2() < 1e-20 || n2.length2() < 1e-20) {
        return false;
    }
    n1.normalize();
    n2.normalize();

    Vec3 bisector = n1 + n2;
    bisector = rejectAxis(bisector, dir);
    if (bisector.length2() < 1e-20) {
        return false;
    }
    bisector.normalize();

    Vec3 into = intoFaceFromEdge(adjacent[0], start, end, dir)
        + intoFaceFromEdge(adjacent[1], start, end, dir);
    into = rejectAxis(into, dir);
    if (into.length2() > 1e-20 && into.dot(bisector) < 0.0) {
        bisector = bisector * -1.0;
    }

    double nDot = std::abs(n1.dot(bisector));
    if (nDot < 1e-6) {
        return false;
    }
    double centerDistance = radius / nDot;

    Vec3 t1 = n1 * -n1.dot(bisector);
    Vec3 t2 = n2 * -n2.dot(bisector);
    if (t1.length2() < 1e-20 || t2.length2() < 1e-20) {
        return false;
    }
    t1.normalize();
    t2.normalize();

    Vec3 toTangent1 = bisector * centerDistance + t1 * radius;
    Vec3 toTangent2 = bisector * centerDistance + t2 * radius;
    double influenceRadius = std::max(toTangent1.length(), toTangent2.length());

    round.start = start;
    round.end = end;
    round.dir = dir;
    round.bisector = bisector;
    round.tangent1 = t1;
    round.tangent2 = t2;
    round.faceIdA = adjacent[0]->getId();
    round.faceIdB = adjacent[1]->getId();
    round.radius = radius;
    round.centerDistance = centerDistance;
    round.influenceRadius = influenceRadius;
    round.aabb = Aabb(start);
    round.aabb.expand(end);
    round.aabb.expandRadius(influenceRadius);
    return true;
}

void rebuildRounds(FaceSubdivisionData& data, const unordered_map<int, Face*>& live) {
    data.rounds.clear();
    unordered_set<int> seen;
    for (const auto& entry : live) {
        Face* face = entry.second;
        if (!face) {
            continue;
        }
        for (HalfEdge* halfEdge : face->getHalfEdges()) {
            if (!halfEdge || !halfEdge->getEdge()) {
                continue;
            }
            Edge* edge = halfEdge->getEdge();
            if (!seen.insert(edge->getId()).second) {
                continue;
            }
            CachedRound round;
            if (fillRound(round, edge, live)) {
                data.rounds[edge->getId()] = round;
            }
        }
    }
}

void applyRound(
    FaceSubdivisionData& data,
    const CachedRound& round,
    double sign,
    const unordered_set<Sample*>* filter
) {
    if (round.radius <= 1e-8) {
        return;
    }
    if (!data.treeValid) {
        rebuildTree(data);
    }

    Vec3 yAxis = round.dir.cross(round.bisector);
    if (yAxis.length2() < 1e-20) {
        return;
    }
    yAxis.normalize();
    double a1 = angleAround(round.tangent1, round.bisector, yAxis);
    double a2 = angleAround(round.tangent2, round.bisector, yAxis);
    double ac = angleAround(round.bisector * -1.0, round.bisector, yAxis);

    data.tree.visitOverlapping(round.aabb, [&](void* payload) {
        Sample* sample = static_cast<Sample*>(payload);
        if (filter && filter->find(sample) == filter->end()) {
            return;
        }
        bool onAdjacent = false;
        for (int faceId : sample->faceIds) {
            if (faceId == round.faceIdA || faceId == round.faceIdB) {
                onAdjacent = true;
                break;
            }
        }
        if (!onAdjacent) {
            return;
        }
        Vec3 closest = closestOnSegment(round.start, round.end, sample->base);
        double edgeDistance = (sample->base - closest).length();
        if (edgeDistance > round.influenceRadius) {
            return;
        }
        Vec3 center = closest + round.bisector * round.centerDistance;
        Vec3 fromCenter = sample->base - center;
        Vec3 along = round.dir * round.dir.dot(fromCenter);
        Vec3 perp = fromCenter - along;
        double dist = perp.length();
        if (dist < round.radius - 1e-8) {
            return;
        }
        if (dist < 1e-12) {
            return;
        }
        if (!angleInArc(angleAround(perp, round.bisector, yAxis), a1, a2, ac)) {
            return;
        }
        Vec3 target = center + along + perp * (round.radius / dist);
        sample->displaced += (target - sample->base) * sign;
    });
}

void applyAllFields(FaceSubdivisionData& data) {
    resetDisplacements(data);
    rebuildTree(data);
    for (const auto& entry : data.faces) {
        applyFace(data, entry.second, 1.0, nullptr);
    }
    for (const auto& entry : data.rounds) {
        applyRound(data, entry.second, 1.0, nullptr);
    }
    data.fieldsApplied = true;
}

}  // namespace

FaceSubdivider::FaceSubdivider()
    : data(std::make_unique<FaceSubdivisionData>()) {
}

FaceSubdivider::~FaceSubdivider() = default;

void FaceSubdivider::clear() {
    data->samples.clear();
    data->edges.clear();
    data->faces.clear();
    data->rounds.clear();
    data->tree.clear();
    data->treeValid = false;
    data->fieldsApplied = false;
}

void FaceSubdivider::sync(const map<int, Face*>& faces, bool deform) {
    unordered_map<int, Face*> live;
    for (const auto& entry : faces) {
        Face* face = entry.second;
        if (!face || face->isHole()) {
            continue;
        }
        live[entry.first] = face;
    }

    vector<int> removed;
    for (const auto& entry : data->faces) {
        auto found = live.find(entry.first);
        if (found == live.end() || faceSignature(found->second) != entry.second.signature) {
            removed.push_back(entry.first);
        }
    }
    for (int faceId : removed) {
        dropFaceGeometry(*data, faceId);
    }

    vector<Face*> added;
    for (const auto& entry : live) {
        if (data->faces.find(entry.first) == data->faces.end()) {
            added.push_back(entry.second);
        }
    }
    for (Face* face : added) {
        addFaceGeometry(*data, face);
    }

    bool changed = !removed.empty() || !added.empty();
    if (!deform) {
        data->fieldsApplied = false;
        return;
    }
    if (!changed && data->fieldsApplied) {
        if (!data->treeValid) {
            rebuildTree(*data);
        }
        return;
    }

    rebuildRounds(*data, live);
    applyAllFields(*data);
}

void FaceSubdivider::append(
    const vector<Face*>& faces,
    vector<Vec3>& positions,
    vector<Vec3>& normals,
    vector<int>& triangles,
    vector<int>& faceIndices,
    bool deform
) const {
    struct BuiltFace {
        int boundaryStart;
        int boundaryCount;
        const CachedFace* item;
    };
    vector<BuiltFace> built;

    auto hasSample = [&](const Key& key) {
        return data->samples.find(key) != data->samples.end();
    };
    auto emit = [&](const Key& key, bool deformPositions, const Vec3& normal) {
        const Sample& sample = data->samples.at(key);
        positions.push_back(deformPositions ? sample.displaced : sample.base);
        normals.push_back(normal);
    };

    // Emit every boundary first so faceIndices stays a partition of outline
    // loops. Interiors come after that; otherwise edge overlays stitch through
    // the grid.
    for (Face* face : faces) {
        if (!face || face->isHole()) {
            continue;
        }
        auto found = data->faces.find(face->getId());
        if (found == data->faces.end()) {
            continue;
        }
        const CachedFace& item = found->second;
        int boundaryCount = static_cast<int>(item.boundaryKeys.size());
        if (boundaryCount < 2) {
            continue;
        }
        bool complete = true;
        for (const Key& key : item.boundaryKeys) {
            if (!hasSample(key)) {
                complete = false;
                break;
            }
        }
        if (complete) {
            for (const Key& key : item.holeKeys) {
                if (!hasSample(key)) {
                    complete = false;
                    break;
                }
            }
        }
        if (complete) {
            for (const Key& key : item.interiorKeys) {
                if (!hasSample(key)) {
                    complete = false;
                    break;
                }
            }
        }
        if (!complete) {
            continue;
        }

        BuiltFace builtFace;
        builtFace.boundaryStart = static_cast<int>(positions.size());
        builtFace.boundaryCount = boundaryCount;
        builtFace.item = &item;
        for (const Key& key : item.boundaryKeys) {
            emit(key, deform, item.normal);
        }
        faceIndices.push_back(static_cast<int>(positions.size()));
        built.push_back(builtFace);
    }

    for (BuiltFace& builtFace : built) {
        const CachedFace& item = *builtFace.item;
        int holeCount = static_cast<int>(item.holeKeys.size());
        int interiorStart = static_cast<int>(positions.size());
        for (const Key& key : item.interiorKeys) {
            emit(key, deform, item.normal);
        }
        int interiorCount = static_cast<int>(item.interiorKeys.size());
        int holeStart = static_cast<int>(positions.size());
        for (const Key& key : item.holeKeys) {
            emit(key, deform, item.normal);
        }

        for (int index = builtFace.boundaryStart; index < builtFace.boundaryStart + builtFace.boundaryCount; index++) {
            normals[index] = Vec3(0, 0, 0);
        }
        for (int index = interiorStart; index < interiorStart + interiorCount; index++) {
            normals[index] = Vec3(0, 0, 0);
        }
        for (int index = holeStart; index < holeStart + holeCount; index++) {
            normals[index] = Vec3(0, 0, 0);
        }

        auto mapIndex = [&](int local) {
            if (local < builtFace.boundaryCount) {
                return builtFace.boundaryStart + local;
            }
            if (local < builtFace.boundaryCount + holeCount) {
                return holeStart + (local - builtFace.boundaryCount);
            }
            return interiorStart + (local - builtFace.boundaryCount - holeCount);
        };
        int localCount = builtFace.boundaryCount + holeCount + interiorCount;
        for (const Tri& triangle : item.triangles) {
            if (triangle.a < 0 || triangle.b < 0 || triangle.c < 0
                || triangle.a >= localCount || triangle.b >= localCount || triangle.c >= localCount) {
                continue;
            }
            int i0 = mapIndex(triangle.a);
            int i1 = mapIndex(triangle.b);
            int i2 = mapIndex(triangle.c);
            Vec3 a = positions[i0];
            Vec3 b = positions[i1];
            Vec3 c = positions[i2];
            Vec3 geometric = (b - a).cross(c - a);
            // Match Face::exportMesh. The emitted winding points opposite the
            // face normal so the faces stay outward after the engines swap Y and Z.
            if (geometric.dot(item.normal) > 0.0) {
                std::swap(i1, i2);
                geometric = geometric * -1.0;
            }
            triangles.push_back(i0);
            triangles.push_back(i1);
            triangles.push_back(i2);
            Vec3 shading = geometric * -1.0;
            normals[i0] += shading;
            normals[i1] += shading;
            normals[i2] += shading;
        }

        auto finishNormal = [&](int index) {
            if (normals[index].length2() < 1e-20) {
                normals[index] = item.normal;
            } else {
                normals[index].normalize();
            }
        };
        for (int index = builtFace.boundaryStart; index < builtFace.boundaryStart + builtFace.boundaryCount; index++) {
            finishNormal(index);
        }
        for (int index = interiorStart; index < interiorStart + interiorCount; index++) {
            finishNormal(index);
        }
        for (int index = holeStart; index < holeStart + holeCount; index++) {
            finishNormal(index);
        }
    }
}
