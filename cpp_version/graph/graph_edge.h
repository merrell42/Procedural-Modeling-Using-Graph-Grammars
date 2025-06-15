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
    void import(const Json& json);

    const vector<vector<GraphHalfEdge*>>& getHalfEdges() const;
    EdgeType* getType() const;
    void setType(EdgeType* newType);

    GraphEdge* connectGraph(Graph* graph);
    // Merge two colinear edges into one.
    void merge(GraphEdge* edgeB, bool mergeForward);

private:
    vector<vector<GraphHalfEdge*>> halfEdges;
    EdgeType* type;
    Graph* graph;
    int id;

    static int nextId;
};
