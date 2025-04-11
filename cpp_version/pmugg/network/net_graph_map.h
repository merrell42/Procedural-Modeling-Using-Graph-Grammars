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

    // Static method to create a NetGraphMap instance
    static NetGraphMap* create(const NetGraphMapInfo& info);

    // Core functionality
    // bool isValid() const { return valid; }
    const std::vector<Vertex*>& getVertexBtoA() const { return vertexBtoA; }
    const std::vector<Line*>& getEdgeBtoA() const { return edgeBtoA; }

    // Operations
    NetGraphMap* copy() const;

    std::vector<Vertex*> vertexBtoA;
    std::vector<Line*> edgeBtoA;
    std::vector<Face*> faceBtoA;

private:
    // bool valid;

    // // Helper methods
    // void initialize(Network* netB, Line* line, bool groundEnabled);
    // bool tryMap(Network* netB, Line* line, bool groundEnabled);
    // bool tryMapVertex(Network* netB, int vertexB, Line* line);
    // bool tryMapEdge(Network* netB, int edgeB, Line* line);
};

} // namespace ms 