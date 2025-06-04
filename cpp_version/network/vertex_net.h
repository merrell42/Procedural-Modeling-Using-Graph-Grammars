#pragma once
#include <vector>
#include <memory>
#include "../guidelines/vertex_type.h"
#include "../third_party/json.h"
#include <iostream>

using Json = nlohmann::json;


namespace ms {

class Network;
class HalfEdgeNet;
class View;
class VertexType;
class EdgeNet;
struct DrawOptions;

class VertexNet {
public:
    VertexNet();
    ~VertexNet() = default;
    void import(const Json& json);

    const std::vector<HalfEdgeNet*>& getHalfEdges() const { return halfEdges; }
    VertexType* getType() {
        if (kind == "e") {
            // The edge case may not be handled correctly.
        }
        return type;
    }
    void setType(VertexType* type_) { type = type_; }
    Network* getNetwork() const { return network; }
    int getId() const { return id; }
    int connectorIndex() const;
    EdgeNet* interiorEdge() const;

    VertexNet* connectNet(Network* network);
    void setHalfEdge(HalfEdgeNet* halfEdge, int index);
    void copyConnection(const VertexNet* copy);

    bool inNetwork() const;

    // TODO: Replace with edgeType.
    std::string kind;

private:
    std::vector<HalfEdgeNet*> halfEdges;
    VertexType* type;
    Network* network;
    int id;

    static int nextId;
};

} // namespace ms 