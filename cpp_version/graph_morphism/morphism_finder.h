#pragma once
#include <vector>
#include <memory>
#include <map>
#include "../geometry/vec2.h"
#include "../graph/graph.h"
#include "../graph_drawing/model.h"
#include "../graph_drawing/face_group.h"

class Model;
class NodeStats;
class Morphism;
class MorphismInfo;
class MorphismState;
class Face;
class Vertex;
class GraphHalfEdge;
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

    IntersectResult() : distance(numeric_limits<double>::infinity()), face(nullptr), data(nullptr) {}
};

class MorphismFinder {
public:
    MorphismFinder(Model* model, bool groundEnabled);
    ~MorphismFinder() = default;

    Morphism* findMorphism(Graph* graphB);
    Morphism* findStarterMorphism(Graph* graphB);
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
    bool groundEnabled;

    Face* findFace(FaceType* faceType);
    Morphism* findContinue(MorphismState* state);
    Morphism* assignVertex(MorphismState* state, Vertex* vertexA, int indexB);
    Morphism* matchHalfEdge(const HalfEdgeData& halfEdgeData, MorphismState* state);
    Morphism* matchSplicedHalfEdge(const HalfEdgeData& halfEdgeData, MorphismState* state);
    Morphism* assignHalfEdge(HalfEdge* halfEdgeA, GraphHalfEdge* halfB, MorphismState* state);

    // Ray casting methods
    /*Face* castVolumeRaySeries(Face* face, const vector<GraphHalfEdge*>& rayHalfs, Face* goalFace);*/
    static IntersectResult castRay(const Vec3& p0, const Vec3& dir, FaceGroup* groupA, const map<int, Face*>& faceMap, int maxDim);
    static HalfEdge* castSeriesOfRays(GraphHalfEdge* halfB, const Vec3& startPos, FaceGroup* groupA, const map<int, Face*>& faceMap, int maxDim);
    static void findFirstIntersection(Face* faceA, const Vec3& p0, const Vec3& p1, const Vec2& dir2, IntersectResult& nearestIntersect, int maxDim);

    // static void addOuterFaces(Morphism* map, Graph* graphB);

    // Cache for spliced vertex types.
    static map<int, VertexType*> splicedVertexTypes;
};

