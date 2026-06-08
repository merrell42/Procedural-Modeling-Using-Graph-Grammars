#include "pch.h"
#include "graph_vertex.h"
#include "graph.h"
#include "graph_half_edge.h"
#include "../util/util.h"
#include "../util/binary_stream.h"

std::atomic<int> GraphVertex::nextId{0};

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
    return indexOf(graph->getBVertices(), const_cast<GraphVertex*>(this));
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

GraphHalfEdge* GraphVertex::interiorHalfEdge() const {
    for (auto* half : halfEdges) {
        if (half->getEdge() != nullptr) {
            return half;
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

Json GraphVertex::exportJson(const vector<GraphHalfEdge*>& graphHalfEdges) const {
    Json json;
    Json halfEdgesJson = Json::array();
    for (auto* half : halfEdges) {
        halfEdgesJson.push_back(half ? indexOf(graphHalfEdges, half) : -1);
    }
    json["halfEdges"] = halfEdgesJson;
    return json;
}

void GraphVertex::binaryDeserialize(std::istream& in) {
    halfEdges.clear();
    auto& graphHalfEdges = graph->getHalfEdges();
    int32_t n = bsRead<int32_t>(in);
    halfEdges.reserve(n);
    for (int32_t i = 0; i < n; i++) {
        int32_t idx = bsRead<int32_t>(in);
        halfEdges.push_back(idx >= 0 ? graphHalfEdges[idx] : nullptr);
    }
}

void GraphVertex::setType(VertexType* newType) {
    type = newType;
}

VertexType* GraphVertex::getType() {
    return type;
}