#pragma once
#include <vector>
#include <memory>
#include <iosfwd>
#include <atomic>
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
    void binaryDeserialize(std::istream& in);
    Json exportJson(const vector<GraphHalfEdge*>& graphHalfEdges) const;

    const vector<GraphHalfEdge*>& getHalfEdges() const { return halfEdges; }
    VertexType* getType();
    void setType(VertexType* newType);
    // The index of the vertex on the graph boundary.
    int boundaryIndex() const;
    // For a boundary vertex return the interior edge / half-edge.
    GraphEdge* interiorEdge() const;
    GraphHalfEdge* interiorHalfEdge() const;

    GraphVertex* connectGraph(Graph* graph);
    void setHalfEdge(GraphHalfEdge* halfEdge, int index);

    // TODO: Replace with edgeType.
    // string kind;

private:
    // All half-edges that originate from this vertex.
    vector<GraphHalfEdge*> halfEdges;
    VertexType* type;
    Graph* graph;
    int id;

    static std::atomic<int> nextId;
};

