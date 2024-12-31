#pragma once
#include <vector>
#include <memory>

namespace ms {

class Network;
class Line;

class NetGraphMap {
public:
    NetGraphMap(Network* netB, Line* line, bool groundEnabled);
    ~NetGraphMap() = default;

    // Core functionality
    bool isValid() const { return valid; }
    const std::vector<int>& getVertexBtoA() const { return vertexBtoA; }
    const std::vector<int>& getEdgeBtoA() const { return edgeBtoA; }

    // Operations
    NetGraphMap* copy() const;

private:
    std::vector<int> vertexBtoA;
    std::vector<int> edgeBtoA;
    bool valid;

    // Helper methods
    void initialize(Network* netB, Line* line, bool groundEnabled);
    bool tryMap(Network* netB, Line* line, bool groundEnabled);
    bool tryMapVertex(Network* netB, int vertexB, Line* line);
    bool tryMapEdge(Network* netB, int edgeB, Line* line);
};

} // namespace ms 