#include "pch.h"
#include "graph.h"
#include "graph_edge.h"
#include "graph_half_edge.h"
#include "../util/util.h"
#include "../util/binary_stream.h"

std::atomic<int> GraphEdge::nextId{0};

GraphEdge::GraphEdge()
    : type(nullptr)
    , graph(nullptr)
    , id(nextId++) {}

const vector<vector<GraphHalfEdge*>>& GraphEdge::getHalfEdges() const {
    return halfEdges;
}

EdgeType* GraphEdge::getType() const {
    return type;
}

GraphEdge* GraphEdge::connectGraph(Graph* newGraph) {
    graph = newGraph;
    graph->addEdge(this);
    return this;
}

void GraphEdge::addHalfEdge(GraphHalfEdge* half, int index) {
    if (index >= (int)halfEdges.size()) {
        halfEdges.resize(index + 1);
    }
    halfEdges[index] = { half };
}

void GraphEdge::removeHalfEdge(GraphHalfEdge* half, int index) {
    if (index >= 0 && index < (int)halfEdges.size() && !halfEdges[index].empty()) {
        if (halfEdges[index][0] == half) {
            halfEdges[index][0] = nullptr;
        }
    }
}

// Merge two colinear edges into one.
void GraphEdge::merge(GraphEdge* edgeB, bool mergeForward) {
    auto* interior = graph;

    auto halfsB = edgeB->getHalfEdges();
    for (size_t edgeIndex = 0; edgeIndex < halfEdges.size(); edgeIndex++) {
        auto* halfA = halfEdges[edgeIndex][0];
        auto* halfB = halfsB[edgeIndex][0];
        auto forward = halfA->getForward();
        
        if (!(forward ^ mergeForward)) {
            // Connect halfA to after halfB.
            auto* nextB = halfB->getNext();
            halfA->disconnect();
            halfB->disconnect();
            halfA->connectNext(nextB);
        } else {
            auto* prevB = halfB->getPrev();
            if (prevB) {
                // Connect halfA to before halfB.
                prevB->disconnect();
                halfB->disconnect();
                prevB->connectNext(halfA);
            }
            halfA->connectVertex(halfB->getVertex(), halfB->getVertexIndex());
        }
        
        auto* face = halfB->getFace();
        face->replaceHalfEdge(halfB, halfA);
        interior->removeHalfEdge(halfB);
    }
    interior->removeEdge(edgeB);
}

void GraphEdge::import(const Json& json) {
    halfEdges.clear();
    auto& graphHalfEdges = graph->getHalfEdges();
    
    if (json.contains("halfEdges")) {
        for (const auto& arrayJson : json["halfEdges"]) {
            vector<GraphHalfEdge*> halfArray;
            for (const auto& index : arrayJson) {
                halfArray.push_back(graphHalfEdges[index.get<int>()]);
            }
            halfEdges.push_back(halfArray);
        }
    }
}

Json GraphEdge::exportJson(const vector<GraphHalfEdge*>& graphHalfEdges) const {
    Json json;
    Json halfEdgesJson = Json::array();
    for (const auto& halfArray : halfEdges) {
        Json arrayJson = Json::array();
        for (auto* half : halfArray) {
            arrayJson.push_back(half ? indexOf(graphHalfEdges, half) : -1);
        }
        halfEdgesJson.push_back(std::move(arrayJson));
    }
    json["halfEdges"] = halfEdgesJson;
    return json;
}

void GraphEdge::binaryDeserialize(std::istream& in) {
    halfEdges.clear();
    auto& graphHalfEdges = graph->getHalfEdges();
    int32_t outerCount = bsRead<int32_t>(in);
    halfEdges.reserve(outerCount);
    for (int32_t i = 0; i < outerCount; i++) {
        int32_t innerCount = bsRead<int32_t>(in);
        vector<GraphHalfEdge*> inner;
        inner.reserve(innerCount);
        for (int32_t j = 0; j < innerCount; j++) {
            inner.push_back(graphHalfEdges[bsRead<int32_t>(in)]);
        }
        halfEdges.push_back(std::move(inner));
    }
}

void GraphEdge::setType(EdgeType* newType) {
    type = newType;
}
