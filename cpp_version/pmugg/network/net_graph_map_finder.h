#pragma once
#include <vector>
#include <memory>
#include "../shape/vec2.h"
#include "../network/network.h"
#include "../graph_drawing/model.h"

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
class Endpoint;
struct EndpointData;

class NetGraphMapFinder {
public:
    NetGraphMapFinder(Model* model, bool groundEnabled);
    ~NetGraphMapFinder() = default;

    // Core functionality
    NetGraphMap* findMap(Network* netB);
    NetGraphMap* findStarterMap(Network* netB);

    // Static configuration
    static constexpr int vertexAttempts = 100;
    static constexpr int faceAttempts = 30;
    static constexpr int spliceRayAttempts = 3;
    static constexpr float maxRayDistance = 10.0f;
    static constexpr float SMALL_DISTANCE = 1e-8f;

private:
    /*struct EndpointData {
        HalfEdge* halfB;
        Vertex* vertexA;
    };*/

    /*struct RayIntersection {
        float distance;
        Face* face;
        IntersectionData data;
    };*/

    Model* model;
    bool nodesModified;
    Face* groundFace;
    bool groundEnabled;

    // Helper methods
    Face* findFace(VertexType* faceType);
    NetGraphMap* findContinue(NetGraphMapState* state);
    NetGraphMap* assignVertex(NetGraphMapState* state, Vertex* vertexA, int indexB);
    NetGraphMap* matchEndpoint(const EndpointData& endpointData, NetGraphMapState* state);
    NetGraphMap* assignEndpoint(Endpoint* endpointA, HalfEdgeNet* halfB, NetGraphMapState* state);
    NetGraphMap* spliceEndpoint(const EndpointData& endpointData, NetGraphMapState* state);

    // Ray casting methods
    /*Face* castVolumeRaySeries(Face* face, const std::vector<HalfEdgeNet*>& rayHalfs, Face* goalFace);
    static RayIntersection castRay(const Vec2& p0, const Vec2& dir, FaceGroup* groupA, int maxDim);
    static Endpoint* castRaySeries(HalfEdgeNet* halfB, const Vec2& startPos, FaceGroup* groupA, int maxDim);*/

    // Static helper methods
    static void addOuterFaces(NetGraphMap* map, Network* netB);
    // static bool neighboringHoles(Line* line, Edge* edgeB);
    static VertexType* getVertexType(EdgeType3D* edgeType);

    // Cache for spliced vertex types
    static std::unordered_map<int, VertexType*> splicedVertexTypes;
};

} // namespace ms 