#pragma once
#include <vector>
#include <memory>
#include "../geometry/vec2.h"
#include "../graph/graph.h"
#include "../graph_drawing/model.h"
#include "../graph_drawing/face_group.h"

namespace ms {

class Model;
class NodeStats;
class NetGraphMap;
class NetGraphMapInfo;
class NetGraphMapState;
class Face;
class Vertex;
class HalfEdgeNet;
class VertexType;
class HalfEdge;
class FaceGroup;
struct HalfEdgeData;
struct IntersectionData;

// Structure to hold intersection results
struct IntersectResult {
    double distance;
    Face* face;
    IntersectionData* data;

    IntersectResult() : distance(std::numeric_limits<double>::infinity()), face(nullptr), data(nullptr) {}
};

class NetGraphMapFinder {
public:
    NetGraphMapFinder(Model* model, bool groundEnabled);
    ~NetGraphMapFinder() = default;

    // Core functionality
    NetGraphMap* findMap(Network* netB);
    NetGraphMap* findStarterMap(Network* netB);
    void reset();

    // Static configuration
    static constexpr int vertexAttempts = 100;
    static constexpr int faceAttempts = 30;
    static constexpr int spliceRayAttempts = 3;
    static constexpr double maxRayDistance = 10.0f;
    static constexpr double SMALL_DISTANCE = 1e-8f;

private:
    Model* model;
    bool nodesModified;
    Face* groundFace;
    bool groundEnabled;

    // Helper methods
    Face* findFace(FaceType3D* faceType);
    NetGraphMap* findContinue(NetGraphMapState* state);
    NetGraphMap* assignVertex(NetGraphMapState* state, Vertex* vertexA, int indexB);
    NetGraphMap* matchHalfEdge(const HalfEdgeData& halfedgeData, NetGraphMapState* state);
    NetGraphMap* assignHalfEdge(HalfEdge* halfedgeA, HalfEdgeNet* halfB, NetGraphMapState* state);
    NetGraphMap* spliceHalfEdge(const HalfEdgeData& halfedgeData, NetGraphMapState* state);

    // Ray casting methods
    /*Face* castVolumeRaySeries(Face* face, const std::vector<HalfEdgeNet*>& rayHalfs, Face* goalFace);*/
    static IntersectResult castRay(const Vec3& p0, const Vec3& dir, FaceGroup* groupA, const std::map<int, Face*>& faceMap, int maxDim);
    static HalfEdge* castRaySeries(HalfEdgeNet* halfB, const Vec3& startPos, FaceGroup* groupA, const std::map<int, Face*>& faceMap, int maxDim);
    static void findNearestIntersection(Face* faceA, const Vec3& p0, const Vec3& p1, const Vec2& dir2, IntersectResult& nearestIntersect, int maxDim);

    // Static helper methods
    static void addOuterFaces(NetGraphMap* map, Network* netB);

    // Cache for spliced vertex types
    static std::unordered_map<int, VertexType*> splicedVertexTypes;
};

} // namespace ms 