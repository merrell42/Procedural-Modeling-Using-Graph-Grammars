#pragma once
#include <vector>
#include <memory>

namespace ms {

class NetGraphMapInfo;
class NetGraphMap;
class GraphHalfEdge;
class Vertex;

struct HalfEdgeData {
    GraphHalfEdge* halfB;
    Vertex* vertexA;

    HalfEdgeData(GraphHalfEdge* half = nullptr, Vertex* vertex = nullptr)
        : halfB(half), vertexA(vertex) {}
};

class NetGraphMapState {
public:
    explicit NetGraphMapState(NetGraphMapInfo* info, NetGraphMap* map = nullptr);
    ~NetGraphMapState() = default;

    NetGraphMapInfo* getInfo() const { return info; }
    NetGraphMap* getMap() { return map; }
    std::vector<HalfEdgeData>& getQueue() { return queue; }
    std::vector<HalfEdgeData>& getSpliceQueue() { return spliceQueue; }

    void setQueue(const std::vector<HalfEdgeData>& newQueue);
    void assignVertex(Vertex* vertexA, int indexB);
    NetGraphMapState* copy() const;

private:
    NetGraphMapInfo* info;
    NetGraphMap* map;
    std::vector<HalfEdgeData> queue;
    std::vector<HalfEdgeData> spliceQueue;
};

} // namespace ms 