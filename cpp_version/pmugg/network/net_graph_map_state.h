#pragma once
#include <vector>
#include <memory>

namespace ms {

class NetGraphMapInfo;
class NetGraphMap;
class HalfEdge;
class Vertex;

class NetGraphMapState {
public:
    explicit NetGraphMapState(NetGraphMapInfo* info, NetGraphMap* map = nullptr);
    ~NetGraphMapState() = default;

    // Core functionality
    NetGraphMapInfo* getInfo() const { return info; }
    NetGraphMap* getMap() const { return map.get(); }
    const std::vector<HalfEdgeData>& getQueue() const { return queue; }
    const std::vector<HalfEdgeData>& getSpliceQueue() const { return spliceQueue; }

    // Operations
    void setQueue(const std::vector<HalfEdgeData>& newQueue);
    void assignVertex(Vertex* vertexA, int indexB);
    void assignHalf(int indexA, int indexB);
    NetGraphMapState* copy() const;

private:
    NetGraphMapInfo* info;
    std::unique_ptr<NetGraphMap> map;
    std::vector<HalfEdgeData> queue;
    std::vector<HalfEdgeData> spliceQueue;

    struct HalfEdgeData {
        HalfEdge* halfB;
        Vertex* vertexA;

        HalfEdgeData(HalfEdge* half = nullptr, Vertex* vertex = nullptr)
            : halfB(half), vertexA(vertex) {}
    };
};

} // namespace ms 