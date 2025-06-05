#include "pch.h"
#include "graph_vertex.h"
#include "graph.h"
#include "graph_half_edge.h"
#include "../util/util.h"

namespace ms {

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

int GraphVertex::connectorIndex() const {
    auto bVertices = graph->getBVertices();
    auto it = std::find(bVertices.begin(), bVertices.end(), this);
    if (it != bVertices.end()) {
        // Compute the index
        return (int)std::distance(bVertices.begin(), it);
    } else {
        return -1;
    }
}

void GraphVertex::copyConnection(const GraphVertex* copy) {
    auto* copyGraph = copy->getGraph();

    halfEdges.clear();
    auto copyHalfEdges = copy->getHalfEdges();
    halfEdges.reserve(copyHalfEdges.size());
    
    for (auto* halfEdge : copyHalfEdges) {
        halfEdges.push_back(halfEdge ? graph->convertHalfEdge(copyGraph, halfEdge) : nullptr);
    }
}

bool GraphVertex::inGraph() const {
    return graph && std::find(graph->getVertices().begin(),
                              graph->getVertices().end(),
                              this) != graph->getVertices().end();
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

} // namespace ms 