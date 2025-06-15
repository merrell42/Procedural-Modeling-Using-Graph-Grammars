#pragma once
#include <vector>
#include <memory>
#include "../primitives/vertex_type.h"
#include "../third_party/json.h"
#include <iostream>

using Json = nlohmann::json;

class Graph;
class GraphHalfEdge;
class View;
class VertexType;
class GraphEdge;
struct DrawOptions;

class GraphVertex {
public:
    GraphVertex();
    ~GraphVertex() = default;
    void import(const Json& json);

    const vector<GraphHalfEdge*>& getHalfEdges() const { return halfEdges; }
    VertexType* getType();
    void setType(VertexType* newType);
    // The index of the vertex on the graph boundary.
    int boundaryIndex() const;
    // Returns the first edge next to the vertex.
    GraphEdge* interiorEdge() const;

    GraphVertex* connectGraph(Graph* graph);
    void setHalfEdge(GraphHalfEdge* halfEdge, int index);

    // TODO: Replace with edgeType.
    string kind;

private:
    vector<GraphHalfEdge*> halfEdges;
    VertexType* type;
    Graph* graph;
    int id;

    static int nextId;
};

