#pragma once
#include <vector>
#include <memory>
#include <cmath>
#include "../graph_grammar.h"
#include "../graph_morphism/morphism.h"
#include "../graph_morphism/morphism_info.h"
#include "../graph_morphism/morphism_path.h"
#include "../placements/vertex_placement.h"
#include "../placements/edge_placement.h"
#include "../placements/face_placement.h"
#include "../util/timer.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>

namespace ms {

struct Production;
class MorphismPath;
class Graph;
class Edge;
class Edge;
class Vertex;
struct FixedFace;

struct EditGraph {
    std::vector<Vertex*> vertices;
    std::vector<Edge*> edges;
    std::vector<Face*> faces;
};

// Helper struct for edge data
struct EdgeData {
    Vec2 v;
    Edge* edge = nullptr;
    double length = 0;
};

struct Limits {
    std::vector<double> min;
    std::vector<double> max;
};

class RuleApplier {
public:
    RuleApplier() = default;

    // Constants
    static constexpr double MAX_ANGLE_DIFFERENCE = (double)(45.0f / 180.0f * M_PI);
    static constexpr int maxEffort = 10;
    static constexpr double precision = 1e-8;
    static constexpr double constraintPrecision = 1e-5;
    static constexpr double minLength = 0;

    // Static factory method
    static std::unique_ptr<RuleApplier> buildNormally(const Production& production, 
                                                       Model* model, int dims);

    bool solve();
    void setup();
    void create(const Production& production, Model* model, int dims);
    void addEdge(Edge* edge, bool includeLength, bool addToGraph);
    EditGraph* createGraph();
    void reject();
    void freeVertex();
    std::vector<double> getExtents();

private:
    Graph* startGraph = nullptr;
    Graph* endGraph = nullptr;
    Morphism* morphism = nullptr;
    bool ground = false;
    int dims = 2;
    
    std::vector<MorphismPath*> openPaths;
    std::vector<EdgeData> edgeData;
    std::vector<Edge*> edges;
    Model* model = nullptr;

    std::vector<Vertex*> freeVertices;
    std::vector<Edge*> freeEdges;
    std::vector<FixedFace> fixedFaces;
    Vec3* initialPosition = nullptr;
    std::vector<int> propagationOrder;
    std::vector<Edge*> basisEdges;
    std::vector<int> fixedVertexIds;
    double effort = 0;
    EditGraph* graph = nullptr;
    double angle = 0;
    std::unique_ptr<RuleApplierSettings> settings;

    // Helper functions
    void addFixedFace(Face* faceA, Face* faceB, double d);
    // bool mergeDuplicateEdges();
    void setupFaceCentric();
    bool sampleSolutionSpace();
    void constrainVertexIds(std::vector<int>& vIds, RuleApplierSettings* settings);
    std::pair<std::vector<double>, bool> sampleFaceCentric();
    std::vector<MorphismPath*> getFreeablePaths() const;
    void freeOneVertex(Vertex* vertex);
    bool placeVertexPositions(const std::vector<double>& positions);
    Limits findLimits();
    bool hasViolations(const std::vector<double>& positions, const Limits& limits);
    Range getRange(const std::vector<int>& orderIds, const std::vector<OrderInfo>& orderInfo);
    void setPlacements(const std::vector<int>& orderIds, const std::vector<OrderInfo>& orderInfo);
};

} // namespace ms 