#pragma once
#include <vector>
#include <memory>
#include "../primitives/vertex_type.h"
#include "../third_party/json.h"
#include <iostream>

using Json = nlohmann::json;


namespace ms {

class Graph;
class GraphHalfEdge;
class View;
class VertexType;
class GraphEdge;
struct DrawOptions;

class VertexNet {
public:
    VertexNet();
    ~VertexNet() = default;
    void import(const Json& json);

    const std::vector<GraphHalfEdge*>& getHalfEdges() const { return halfEdges; }
    VertexType* getType() {
        if (kind == "e") {
            // The edge case may not be handled correctly.
        }
        return type;
    }
    void setType(VertexType* type_) { type = type_; }
    Graph* getGraph() const { return graph; }
    int getId() const { return id; }
    int connectorIndex() const;
    GraphEdge* interiorEdge() const;

    VertexNet* connectNet(Graph* graph);
    void setHalfEdge(GraphHalfEdge* halfEdge, int index);
    void copyConnection(const VertexNet* copy);

    bool inGraph() const;

    // TODO: Replace with edgeType.
    std::string kind;

private:
    std::vector<GraphHalfEdge*> halfEdges;
    VertexType* type;
    Graph* graph;
    int id;

    static int nextId;
};

} // namespace ms 