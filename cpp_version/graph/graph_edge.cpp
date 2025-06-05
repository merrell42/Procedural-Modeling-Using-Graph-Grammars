#include "pch.h"
#include "graph.h"
#include "graph_edge.h"
#include "graph_half_edge.h"
#include "../util/util.h"

namespace ms {

int GraphEdge::nextId = 0;

GraphEdge::GraphEdge()
    : type(nullptr)
    , graph(nullptr)
    , id(nextId++) {}

const std::vector<std::vector<GraphHalfEdge*>>& GraphEdge::getHalfEdges() const {
    return halfEdges;
}

Graph* GraphEdge::getGraph() const {
    return graph;
}

int GraphEdge::getId() const {
    return id;
}

GraphEdge* GraphEdge::connectNet(Graph* net) {
    graph = net;
    graph->addEdge(this);
    return this;
}

void GraphEdge::addHalfEdge(GraphHalfEdge* halfEdge, int index) {
    if (index >= halfEdges.size()) {
        halfEdges.resize(index + 1);
    }
    halfEdges[index].push_back(halfEdge);
}

void GraphEdge::removeHalfEdge(GraphHalfEdge* halfEdge, int index) {
    if (index < halfEdges.size()) {
        halfEdges.erase(halfEdges.begin() + index);
    }
}

void GraphEdge::copyConnection(const GraphEdge* copy) {
    auto* copyNet = copy->getGraph();
    auto copyHalfEdges = copy->getHalfEdges();
    
    halfEdges.clear();
    halfEdges.resize(copyHalfEdges.size());
    
    for (size_t i = 0; i < copyHalfEdges.size(); i++) {
        for (auto* half : copyHalfEdges[i]) {
            halfEdges[i].push_back(graph->convertHalfEdge(copyNet, half));
        }
    }
}

bool GraphEdge::inGraph() const {
    auto vec = graph->getEdges();
    return std::find(vec.begin(), vec.end(), this) != vec.end();
}

void GraphEdge::merge(GraphEdge* edgeB, bool mergeForward) {
    auto* interior = graph;

    auto halfsB = edgeB->getHalfEdges();
    for (size_t edgeIndex = 0; edgeIndex < halfEdges.size(); edgeIndex++) {
        auto* halfA = halfEdges[edgeIndex][0];
        auto* halfB = halfsB[edgeIndex][0];
        auto forward = halfA->getForward();
        
        if (!(forward ^ mergeForward)) {
            auto* nextB = halfB->getNext();
            halfA->disconnectHalfEdge();
            halfB->disconnectHalfEdge();
            halfA->connectHalfEdge(nextB);
        } else {
            auto* prevB = halfB->getPrev();
            if (prevB) {
                prevB->disconnectHalfEdge();
                halfB->disconnectHalfEdge();
                prevB->connectHalfEdge(halfA);
            }
            halfA->connectVertex(halfB->getVertex(), halfB->getVertexIndex());
        }
        
        auto* face = halfB->getFace();
        face->replaceHalfEdge(halfB, halfA, false);
        interior->removeHalfEdge(halfB);
    }
    interior->removeEdge(edgeB);
}

void GraphEdge::import(const Json& json) {
    halfEdges.clear();
    auto& graphHalfEdges = graph->getHalfEdges();
    
    if (json.contains("halfEdges")) {
        for (const auto& arrayJson : json["halfEdges"]) {
            std::vector<GraphHalfEdge*> halfArray;
            for (const auto& index : arrayJson) {
                halfArray.push_back(graphHalfEdges[index.get<int>()]);
            }
            halfEdges.push_back(halfArray);
        }
    }
}

} // namespace ms 