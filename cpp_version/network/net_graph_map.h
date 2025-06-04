#pragma once
#include <vector>
#include <memory>
#include "net_graph_map_info.h"

namespace ms {

class Network;
class Line;

class NetGraphMap {
public:
    NetGraphMap();
    ~NetGraphMap() = default;
    NetGraphMap* copy() const;
    static NetGraphMap* create(const NetGraphMapInfo& info);

    const std::vector<Vertex*>& getVertexBtoA() const { return vertexBtoA; }
    const std::vector<Line*>& getEdgeBtoA() const { return edgeBtoA; }

    std::vector<Vertex*> vertexBtoA;
    std::vector<Line*> edgeBtoA;
    std::vector<Face*> faceBtoA;
};

} // namespace ms 