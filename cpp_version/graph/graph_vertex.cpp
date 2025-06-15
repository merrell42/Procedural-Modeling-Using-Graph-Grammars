#include "pch.h"
#include "graph_vertex.h"
#include "graph.h"
#include "graph_half_edge.h"
#include "../util/util.h"

int GraphVertex::nextId = 0;

GraphVertex::GraphVertex()
    : graph(nullptr)
    , type(nullptr)
    , id(nextId++) {}

GraphVertex* GraphVertex::connectGraph(Graph* newGraph) {
    graph = newGraph;
    graph->addVertex(this);
    return this;
}

void GraphVertex::setHalfEdge(GraphHalfEdge* halfEdge, int index) {
    if (index >= halfEdges.size()) {
        halfEdges.resize(index + 1, nullptr);
    }
    halfEdges[index] = halfEdge;
}

int GraphVertex::boundaryIndex() const {
    return indexOf(graph->getBVertices(), this);
}

GraphEdge* GraphVertex::interiorEdge() const {
    for (auto* halfEdge : halfEdges) {
        if (halfEdge) {
            auto edge = halfEdge->getEdge();
            if (edge) {
                return edge;
            }
        }
    }
    return nullptr;
}

void GraphVertex::import(const Json& json) {
    halfEdges.clear();
    auto& graphHalfEdges = graph->getHalfEdges();
    
    if (json.contains("halfEdges")) {
        for (const auto& index : json["halfEdges"]) {
            int idx = index.get<int>();
            halfEdges.push_back(idx >= 0 ? graphHalfEdges[idx] : nullptr);
        }
    }
}

void GraphVertex::setType(VertexType* newType) {
    type = newType;
}

VertexType* GraphVertex::getType() {
    return type;
}