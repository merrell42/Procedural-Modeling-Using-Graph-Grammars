#pragma once
#include <vector>
#include <memory>
#include "../memory_counter.h"

using namespace std;

class MorphismInfo;
class Morphism;
class GraphHalfEdge;
class Vertex;

// One of vertex A's half-edges will need to be assigned to half B.
struct HalfEdgeData {
    GraphHalfEdge* halfB;
    Vertex* vertexA;

    HalfEdgeData(GraphHalfEdge* half = nullptr, Vertex* vertex = nullptr)
        : halfB(half), vertexA(vertex) {}
};

// The state of morphism finder.
class MorphismState {
public:
    explicit MorphismState(MorphismInfo* info);
    ~MorphismState();

    MorphismInfo* getInfo() const;
    Morphism* getMorphism();
    vector<HalfEdgeData>& getQueue();
    vector<HalfEdgeData>& getSpliceQueue();

    void assignVertex(Vertex* vertexA, int indexB);

private:
    MorphismInfo* info;
    Morphism* morphism;

    // A queue of half-edges left to process.
    vector<HalfEdgeData> queue;
    vector<HalfEdgeData> spliceQueue;
};

