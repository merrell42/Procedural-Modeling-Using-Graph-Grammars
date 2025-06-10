#pragma once
#include <vector>
#include <memory>
#include "../third_party/json.h"

using Json = nlohmann::json;

class Graph;
class GraphHalfEdge;
class View;
class EdgeType;

class GraphEdge {
public:
    GraphEdge();
    ~GraphEdge() = default;

    const vector<vector<GraphHalfEdge*>>& getHalfEdges() const;
    EdgeType* getType();
    void setType(EdgeType* type_);

    GraphEdge* connectGraph(Graph* graph);
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
