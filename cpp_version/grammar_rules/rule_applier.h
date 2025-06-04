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

struct Transition;
class TransistorPath;
class Network;
class Line;
class Edge;
class Vertex;
struct FixedFace;

struct Graph {
    std::vector<Vertex*> vertices;
    std::vector<Line*> edges;
    std::vector<Face*> faces;
};

// Helper struct for line data
struct LineData {
    Vec2 v;
    Line* line = nullptr;
    double length = 0;
};

struct Limits {
    std::vector<double> min;
    std::vector<double> max;
};

class NetTransistor {
public:
    NetTransistor() = default;

    // Constants
    static constexpr double MAX_ANGLE_DIFFERENCE = (double)(45.0f / 180.0f * M_PI);
    static constexpr int maxEffort = 10;
    static constexpr double precision = 1e-8;
    static constexpr double constraintPrecision = 1e-5;
    static constexpr double minLength = 0;

    // Static factory method
    static std::unique_ptr<NetTransistor> buildNormally(const Transition& transition, 
                                                       Model* model, int dims);

    bool solve();
    void setup();
    void create(const Transition& transition, Model* model, int dims);
    void addLine(Line* line, bool includeLength, bool addToGraph);
    Graph* createGraph();
    void reject();
    void freeVertex();
    std::vector<double> getExtents();

private:
    Network* startNet = nullptr;
    Network* endNet = nullptr;
    NetGraphMap* map = nullptr;
    bool ground = false;
    int dims = 2;
    
    std::vector<TransistorPath*> openPaths;
    std::vector<LineData> lineData;
    std::vector<Line*> lines;
    Model* model = nullptr;

    std::vector<Vertex*> freeVertices;
    std::vector<Edge*> freeEdges;
    std::vector<FixedFace> fixedFaces;
    Vec3* initialPosition = nullptr;
    std::vector<int> propagationOrder;
    std::vector<Edge*> basisEdges;
    std::vector<int> fixedVertexIds;
    double effort = 0;
    Graph* graph = nullptr;
    double angle = 0;
    std::unique_ptr<NetTransistorSettings> settings;

    // Helper functions
    void addFixedFace(Face* faceA, Face* faceB, double d);
    // bool mergeDuplicateLines();
    void setupFaceCentric();
    bool sampleSolutionSpace();
    void constrainVertexIds(std::vector<int>& vIds, NetTransistorSettings* settings);
    std::pair<std::vector<double>, bool> sampleFaceCentric();
    std::vector<TransistorPath*> getFreeablePaths() const;
    void freeOneVertex(Vertex* vertex);
    bool placeVertexPositions(const std::vector<double>& positions);
    Limits findLimits();
    bool hasViolations(const std::vector<double>& positions, const Limits& limits);
    Range getRange(const std::vector<int>& orderIds, const std::vector<OrderInfo>& orderInfo);
    void setPlacements(const std::vector<int>& orderIds, const std::vector<OrderInfo>& orderInfo);
};

} // namespace ms 