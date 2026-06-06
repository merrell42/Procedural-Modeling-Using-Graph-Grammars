#pragma once
#include <vector>
#include <memory>
#include <iosfwd>
#include <atomic>
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
    void binaryDeserialize(std::istream& in);

    const vector<vector<GraphHalfEdge*>>& getHalfEdges() const;
    EdgeType* getType() const;
    void setType(EdgeType* newType);

    // Connect the graph and edge together.
    GraphEdge* connectGraph(Graph* graph);
    void addHalfEdge(GraphHalfEdge* half, int index);
    void removeHalfEdge(GraphHalfEdge* half, int index);
    // Merge two colinear edges into one.
    void merge(GraphEdge* edgeB, bool mergeForward);

private:
    vector<vector<GraphHalfEdge*>> halfEdges;
    EdgeType* type;
    Graph* graph;
    int id;

    static std::atomic<int> nextId;
};
