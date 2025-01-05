#pragma once
#include <vector>
#include <memory>
#include <cmath>
#include "../network/net_graph_map.h"
#include "../network/net_graph_map_info.h"
#include "../network/vertex_placement.h"
#include "../network/edge_placement.h"
#include "../network/face_placement.h"
#include "../hierarchy/network_hierarchy.h"
// #include "../math/math.h"
#include "../fragment/transistor_path.h"
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





// Helper struct for line data
struct LineData {
    Vec2 v;
    Line* line = nullptr;
    double length = 0;
};

class NetTransistor {
public:
    // Constants
    static constexpr float MAX_ANGLE_DIFFERENCE = (float)(45.0f / 180.0f * M_PI);
    static constexpr int maxEffort = 10;
    static constexpr double precision = 1e-8;
    static constexpr double constraintPrecision = 1e-5;
    static constexpr double minLength = 0;

    // Static factory method
    static std::unique_ptr<NetTransistor> buildNormally(const Transition& transition, 
                                                       NodeStats* nodeStats, 
                                                       bool is3D);

    // Constructor
    NetTransistor() = default;

    // Member functions
    void create(const Transition& transition, NodeStats* nodeStats, bool is3D);
    void addLine(Line* line, bool includeLength, bool addToGraph);
    Graph* createGraph();
    void setupFaceCentric(/*const MutationArea& mutationArea*/);
    void reject();

private:
    // Member variables
    Network* startNet = nullptr;
    Network* endNet = nullptr;
    std::unique_ptr<NetGraphMap> map;
    /*Ground* ground = nullptr;*/
    int dims = 2;
    
    std::vector<TransistorPath*> openPaths;
    std::vector<LineData> lineData;
    std::vector<Line*> lines;
    NodeStats* stats = nullptr;
    Model* model = nullptr;

    std::vector<Vertex*> freeVertices;
    std::vector<Edge*> freeEdges;
    // std::vector<EdgeBlocker*> edgeBlockers;
    // Matrix* changeBasis = nullptr;
    Vec3* initialPosition = nullptr;
    std::vector<int> propagationOrder;
    std::vector<Edge*> basisEdges;
    bool isInitialBoundary = false;
    double effort = 0;
    Graph* graph = nullptr;
    float angle = 0;

    // Helper functions
    void addFixedFace(Face* faceA, Face* faceB, double d);
    bool mergeDuplicateLines();
};

} // namespace ms 