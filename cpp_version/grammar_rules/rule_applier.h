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
#include <math.h>

struct Transition;
class MorphismPath;
class Graph;
class Edge;
class Edge;
class Vertex;
struct FixedFace;

struct EditGraph {
    vector<Vertex*> vertices;
    vector<Edge*> edges;
    vector<Face*> faces;
};

// Helper struct for edge data
struct EdgeData {
    Vec2 v;
    Edge* edge = nullptr;
    double length = 0;
};

struct Limits {
    vector<double> min;
    vector<double> max;
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
    static unique_ptr<RuleApplier> buildNormally(const Transition& transition, 
                                                       Model* model, int dims);

    bool solve();
    void setup();
    void create(const Transition& transition, Model* model, int dims);
    void addEdge(Edge* edge, bool includeLength, bool addToGraph);
    EditGraph* createGraph();
    void reject();
    void freeVertex();
    vector<double> getExtents();

private:
    Graph* startNet = nullptr;
    Graph* endNet = nullptr;
    Morphism* map = nullptr;
    bool ground = false;
    int dims = 2;
    
    vector<MorphismPath*> openPaths;
    vector<EdgeData> edgeData;
    vector<Edge*> edges;
    Model* model = nullptr;

    vector<Vertex*> freeVertices;
    vector<Edge*> freeEdges;
    vector<FixedFace> fixedFaces;
    Vec3* initialPosition = nullptr;
    vector<int> propagationOrder;
    vector<Edge*> basisEdges;
    vector<int> fixedVertexIds;
    double effort = 0;
    EditGraph* graph = nullptr;
    double angle = 0;
    unique_ptr<RuleApplierSettings> settings;

    // Helper functions
    void addFixedFace(Face* faceA, Face* faceB, double d);
    // bool mergeDuplicateEdges();
    void setupFaceCentric();
    bool sampleSolutionSpace();
    void constrainVertexIds(vector<int>& vIds, RuleApplierSettings* settings);
    pair<vector<double>, bool> sampleFaceCentric();
    vector<MorphismPath*> getFreeablePaths() const;
    void freeOneVertex(Vertex* vertex);
    bool placeVertexPositions(const vector<double>& positions);
    Limits findLimits();
    bool hasViolations(const vector<double>& positions, const Limits& limits);
    Range getRange(const vector<int>& orderIds, const vector<OrderInfo>& orderInfo);
    void setPlacements(const vector<int>& orderIds, const vector<OrderInfo>& orderInfo);
};

