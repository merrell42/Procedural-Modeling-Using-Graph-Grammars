#pragma once
#include <vector>
#include <memory>
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

class Graph;
class GraphHalfEdge;
class View;
class EdgeType;

class GraphEdge {
public:
    GraphEdge();
    ~GraphEdge() = default;

    // Core functionality
    const vector<vector<GraphHalfEdge*>>& getHalfEdges() const;
    EdgeType* getType() { return type; }
    void setType(EdgeType* type_) { type = type_; }
    Graph* getGraph() const;
    int getId() const;

    GraphEdge* connectGraph(Graph* graph);
    void addHalfEdge(GraphHalfEdge* halfEdge, int index);
    void removeHalfEdge(GraphHalfEdge* halfEdge, int index);
    void copyConnection(const GraphEdge* copy);

    // Validation
    bool inGraph() const;
    void merge(GraphEdge* edgeB, bool mergeForward);
    void import(const Json& json);

private:
    vector<vector<GraphHalfEdge*>> halfEdges;
    EdgeType* type;
    Graph* graph;
    int id;

    static int nextId;
};

} // namespace ms 