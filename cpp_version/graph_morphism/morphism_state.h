#pragma once
#include <vector>
#include <memory>

namespace ms {

class NetGraphMapInfo;
class NetGraphMap;
class HalfEdgeNet;
class Vertex;

struct HalfEdgeData {
    HalfEdgeNet* halfB;
    Vertex* vertexA;

    HalfEdgeData(HalfEdgeNet* half = nullptr, Vertex* vertex = nullptr)
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