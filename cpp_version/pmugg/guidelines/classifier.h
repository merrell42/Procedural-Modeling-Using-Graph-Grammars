#pragma once
#include "../shape/vec2.h"
#include "../hierarchy/network_hierarchy.h"
// #include "family_tree.h"
#include <vector>
#include <memory>

namespace ms {

class Classifier {
public:
    Classifier();
    ~Classifier();

    // Core functionality
    void importSolution(const Solution& solution);
    void resetHierarchy();
    void setShapes(const std::vector<Shape*>& shapes);
    FamilyTree* getFamilyTree();
    std::vector<BoundaryGroup*> getBoundaryGroups();
    bool isFullyConnected() const;

    // Static helper functions
    static float ANGLE_TOLERANCE;
    static float FLIP_ANGLE;
    static EdgeDatum* findMatchingEdge(const EdgeDatum& edgeDatumA, 
                                     const std::vector<EdgeDatum>& edgeDataB,
                                     bool isRigidFixed);
    static void snapEdgeAngles(std::vector<EdgeType*>& edgeTypes);
    static bool switchDirections(Edge* edge);
    static std::vector<VertexData> extractPrimitives2D(const std::vector<Shape*>& shapes);
    static std::vector<VertexData> removeRedundantVertexTypes(std::vector<VertexData>& vertexData);
    static std::vector<std::string> vertexIdentifier(VertexType* vertexType);
    static float edgeAngle(Vertex* startVertex, Vertex* endVertex);

private:
    std::vector<Edge*> edges;  // Note: edges is never needed
    std::unique_ptr<NetworkHierarchy> hierarchy;
    std::vector<BoundaryGroup*> boundaryGroups;
    bool fullyConnected;
    bool is3D;
    XmlData* xml;
};

// Helper structs
struct VertexData {
    Vec2 position;
    VertexType* vertexType;
    std::vector<std::string> edges;
};

struct EdgeDatum {
    float angle;
    Brush* brush;
    Edge* shapeEdge;
    EdgeType* edgeType;
    Vertex* startVertex;
    Vertex* endVertex;
    Area* leftArea;
    Area* rightArea;
};

} // namespace ms 