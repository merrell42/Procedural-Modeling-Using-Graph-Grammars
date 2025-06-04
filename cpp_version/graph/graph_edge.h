#pragma once
#include <vector>
#include <memory>
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

class Network;
class HalfEdgeNet;
class View;
class EdgeType3D;

class EdgeNet {
public:
    EdgeNet();
    ~EdgeNet() = default;

    // Core functionality
    const std::vector<std::vector<HalfEdgeNet*>>& getHalfEdges() const;
    EdgeType3D* getType() { return type; }
    void setType(EdgeType3D* type_) { type = type_; }
    Network* getNetwork() const;
    int getId() const;

    EdgeNet* connectNet(Network* network);
    void addHalfEdge(HalfEdgeNet* halfEdge, int index);
    void removeHalfEdge(HalfEdgeNet* halfEdge, int index);
    void copyConnection(const EdgeNet* copy);

    // Validation
    bool inNetwork() const;
    void merge(EdgeNet* edgeB, bool mergeForward);
    void import(const Json& json);

private:
    std::vector<std::vector<HalfEdgeNet*>> halfEdges;
    EdgeType3D* type;
    Network* network;
    int id;

    static int nextId;
};

} // namespace ms 