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
    vector<HalfEdge*> halfEdges;
};

struct Limits {
    vector<double> min;
    vector<double> max;
};

class RuleApplier {
public:
    RuleApplier();
    ~RuleApplier();

    // Constants
    static constexpr double MAX_ANGLE_DIFFERENCE = (double)(45.0f / 180.0f * M_PI);
    static constexpr int maxEffort = 10;
    static constexpr double precision = 1e-8;
    static constexpr double constraintPrecision = 1e-5;
    static constexpr double minLength = 0;

    // Static factory method
    static unique_ptr<RuleApplier> build(const Production& production, Model* model, int dims);
    void create(const Production& production, Model* model, int dims);

    bool solve();
    void addEdgeToGraph(Edge* edge);
    EditGraph* createGraph();
    void reject();
    Vertex* pickVertexToFree();
    void freeVertex(Vertex* vertex);
    vector<double> getExtents();

private:
    Graph* startGraph = nullptr;
    Graph* endGraph = nullptr;
    // The morphism matches the start graph to the graph drawing.
    Morphism* morphism = nullptr;
    EditGraph* graph = nullptr;
    Model* model = nullptr;

    bool ground = false;
    int dims = 2;

    // Paths around the faces that we are editing that can be expanded.
    vector<MorphismPath*> openPaths;

    // freeVertices are the same as graph->vertices with the empty vertices removed.
    vector<Vertex*> freeVertices;

    // Fixed vertices and faces are ones that cannot be moved.
    vector<int> fixedVertexIds;
    vector<FixedFace> fixedFaces;

    // Effort is the number of samples we've tried.
    double effort = 0;
    unique_ptr<RuleApplierSettings> settings;

    // Helper functions
    void addFixedFace(Face* faceA, Face* faceB, double d);
    // bool mergeDuplicateEdges();
    void setupForSampling();
    bool sampleRepeatedly();
    void constrainVertexIds(vector<int>& vIds, RuleApplierSettings* settings);
    pair<vector<double>, bool> sampleSolutionSpace();
    vector<MorphismPath*> getFreeablePaths() const;
    void freeVertexBase(Vertex* vertex);
    bool placeVertexPositions(const vector<double>& positions);
    bool outOfBounds(const vector<double>& positions);
    Range getRange(const vector<int>& orderIds, const vector<OrderInfo>& orderInfo, int startIndex, int endIndex);
    void setPlacements(const vector<int>& orderIds, const vector<OrderInfo>& orderInfo, int startIndex, int endIndex);
    pair<vector<double>, bool> createGroundPlane();
};

