#pragma once
#include <vector>
#include <memory>
#include "../third_party/json.h"
#include "primal_edge.h"

using Json = nlohmann::json;

namespace ms {

class Network;
class HalfEdgeNet;
class View;
class PrimalEdge;

class EdgeNet {
public:
    EdgeNet();
    ~EdgeNet() = default;

    // Core functionality
    const std::vector<std::vector<HalfEdgeNet*>>& getHalfEdges() const;
    PrimalEdge* getPrimal() const;
    Network* getNetwork() const;
    int getId() const;

    // Network operations
    void setPrimal(PrimalEdge* primal);
    EdgeNet* connectNet(Network* network);
    void addHalfEdge(HalfEdgeNet* halfEdge, int index);
    void removeHalfEdge(HalfEdgeNet* halfEdge, int index);
    void copyConnection(const EdgeNet* copy);

    // Validation
    bool inNetwork() const;

    // Drawing
    void highlight(View* view);
    void print() const;
    bool requiresShapeView() const;

    // Edge operations
    void merge(EdgeNet* edgeB, bool mergeForward);

    // Import/Export
    // Json export() const;
    void import(const Json& json);

private:
    std::vector<std::vector<HalfEdgeNet*>> halfEdges;
    PrimalEdge* primal;
    Network* network;
    int id;

    static int nextId;
};

} // namespace ms 