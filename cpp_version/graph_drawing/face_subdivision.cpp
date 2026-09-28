#include "pch.h"
#include "face_subdivision.h"
#include "face.h"
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
        vector<Key> interiorKeys;
        vector<Tri> triangles;
    };

    unordered_map<Key, Sample, KeyHash> samples;
    unordered_map<int, EdgeInfo> edges;
    unordered_map<int, CachedFace> faces;
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
    bool create
) {
    vector<BoundaryPoint> boundary;
    auto pushPoint = [&](const Key& key) {
        auto found = data.samples.find(key);
        if (found == data.samples.end()) {
            return;
        }
        if (create) {
            addFaceRef(found->second.faceIds, face->getId());
        }
        const Vec3& base = found->second.base;
        if (!boundary.empty()) {
            auto previous = data.samples.find(boundary.back().key);
            if (previous != data.samples.end() && (base - previous->second.base).length2() < 1e-16) {
                return;
            }
        }
        boundary.push_back(BoundaryPoint{key, Vec2(u.dot(base), face->getFaceType()->getV().dot(base))});
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
        EdgeInfo& info = ensureEdge(data, halfEdge, face->getId());
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
        using Point = std::array<double, 2>;
        vector<vector<Point>> ring(1);
        for (const Vec2& point : polygon) {
            ring[0].push_back({point.x, point.y});
        }
        auto indices = mapbox::earcut(ring);
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            cached.triangles.push_back(Tri{
                static_cast<int>(indices[i]),
                static_cast<int>(indices[i + 1]),
                static_cast<int>(indices[i + 2])
            });
        }
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
            if (face->containsPoint(world)) {
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
        Vec2 placed = face->containsPoint(jitteredWorld) ? jittered : point;
        Vec3 base = axisU * placed.x + axisV * placed.y + normal * plane;
        int local = boundaryCount + static_cast<int>(cached.interiorKeys.size());
        Key key = interiorKey(face->getId(), static_cast<int>(cached.interiorKeys.size()));
        data.samples.emplace(key, makeSample(base));
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
                for (int candidate : outs->second) {
                    if (unused.count({to, candidate})) {
                        next = candidate;
                        break;
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
    for (HalfEdge* halfEdge : face->getHalfEdges()) {
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
    vector<BoundaryPoint> boundary = traceBoundary(data, face, u, true);
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
    for (const Key& key : cached.boundaryKeys) {
        auto sample = data.samples.find(key);
        if (sample == data.samples.end()) {
            continue;
        }
        removeFaceRef(sample->second.faceIds, faceId);
        if (sample->second.faceIds.empty()) {
            data.samples.erase(sample);
        }
    }
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

void applyAllFaces(FaceSubdivisionData& data) {
    resetDisplacements(data);
    rebuildTree(data);
    for (const auto& entry : data.faces) {
        applyFace(data, entry.second, 1.0, nullptr);
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

    if (!removed.empty()) {
        if (data->fieldsApplied) {
            if (!data->treeValid) {
                rebuildTree(*data);
            }
            for (int faceId : removed) {
                applyFace(*data, data->faces[faceId], -1.0, nullptr);
            }
        }
        for (int faceId : removed) {
            dropFaceGeometry(*data, faceId);
        }
    }

    vector<Face*> added;
    for (const auto& entry : live) {
        if (data->faces.find(entry.first) == data->faces.end()) {
            added.push_back(entry.second);
        }
    }

    if (!deform) {
        for (Face* face : added) {
            addFaceGeometry(*data, face);
        }
        data->fieldsApplied = false;
        return;
    }

    if (!data->fieldsApplied) {
        for (Face* face : added) {
            addFaceGeometry(*data, face);
        }
        applyAllFaces(*data);
        return;
    }

    if (added.empty()) {
        if (!data->treeValid) {
            rebuildTree(*data);
        }
        return;
    }

    unordered_set<Key, FaceSubdivisionData::KeyHash> before;
    before.reserve(data->samples.size());
    for (const auto& entry : data->samples) {
        before.insert(entry.first);
    }
    for (Face* face : added) {
        addFaceGeometry(*data, face);
    }
    unordered_set<Sample*> created;
    for (auto& entry : data->samples) {
        if (before.find(entry.first) == before.end()) {
            created.insert(&entry.second);
        }
    }
    rebuildTree(*data);

    unordered_set<int> addedIds;
    for (Face* face : added) {
        addedIds.insert(face->getId());
    }
    for (const auto& entry : data->faces) {
        if (addedIds.count(entry.first)) {
            applyFace(*data, entry.second, 1.0, nullptr);
        } else {
            applyFace(*data, entry.second, 1.0, &created);
        }
    }
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
        int interiorStart = static_cast<int>(positions.size());
        for (const Key& key : item.interiorKeys) {
            emit(key, deform, item.normal);
        }
        int interiorCount = static_cast<int>(item.interiorKeys.size());

        for (int index = builtFace.boundaryStart; index < builtFace.boundaryStart + builtFace.boundaryCount; index++) {
            normals[index] = Vec3(0, 0, 0);
        }
        for (int index = interiorStart; index < interiorStart + interiorCount; index++) {
            normals[index] = Vec3(0, 0, 0);
        }

        auto mapIndex = [&](int local) {
            if (local < builtFace.boundaryCount) {
                return builtFace.boundaryStart + local;
            }
            return interiorStart + (local - builtFace.boundaryCount);
        };
        int localCount = builtFace.boundaryCount + interiorCount;
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
    }
}
