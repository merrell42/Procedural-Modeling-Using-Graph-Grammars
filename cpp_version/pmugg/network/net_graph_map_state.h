#pragma once
#include <vector>
#include <memory>

namespace ms {

class NetGraphMapInfo;
class NetGraphMap;
class HalfEdgeNet;
class Vertex;

struct EndpointData {
    HalfEdgeNet* halfB;
    Vertex* vertexA;

    EndpointData(HalfEdgeNet* half = nullptr, Vertex* vertex = nullptr)
        : halfB(half), vertexA(vertex) {}
};

class NetGraphMapState {
public:
    explicit NetGraphMapState(NetGraphMapInfo* info, NetGraphMap* map = nullptr);
    ~NetGraphMapState() = default;

    // Core functionality
    NetGraphMapInfo* getInfo() const { return info; }
    NetGraphMap* getMap() const { return map.get(); }
    std::vector<EndpointData>& getQueue() { return queue; }
    std::vector<EndpointData>& getSpliceQueue() { return spliceQueue; }

    // Operations
    void setQueue(const std::vector<EndpointData>& newQueue);
    void assignVertex(Vertex* vertexA, int indexB);
    void assignHalf(int indexA, int indexB);
    NetGraphMapState* copy() const;

private:
    NetGraphMapInfo* info;
    std::unique_ptr<NetGraphMap> map;
    std::vector<EndpointData> queue;
    std::vector<EndpointData> spliceQueue;
};

} // namespace ms 