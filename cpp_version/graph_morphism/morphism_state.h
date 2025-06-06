#pragma once
#include <vector>
#include <memory>

using namespace std;

class MorphismInfo;
class Morphism;
class GraphHalfEdge;
class Vertex;

struct HalfEdgeData {
    GraphHalfEdge* halfB;
    Vertex* vertexA;

    HalfEdgeData(GraphHalfEdge* half = nullptr, Vertex* vertex = nullptr)
        : halfB(half), vertexA(vertex) {}
};

class MorphismState {
public:
    explicit MorphismState(MorphismInfo* info, Morphism* morphism = nullptr);
    ~MorphismState() = default;

    MorphismInfo* getInfo() const { return info; }
    Morphism* getMorphism() { return morphism; }
    vector<HalfEdgeData>& getQueue() { return queue; }
    vector<HalfEdgeData>& getSpliceQueue() { return spliceQueue; }

    void setQueue(const vector<HalfEdgeData>& newQueue);
    void assignVertex(Vertex* vertexA, int indexB);
    MorphismState* copy() const;

private:
    MorphismInfo* info;
    Morphism* morphism;
    vector<HalfEdgeData> queue;
    vector<HalfEdgeData> spliceQueue;
};

