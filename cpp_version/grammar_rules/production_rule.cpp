#include "pch.h"
#include <atomic>
#include "production_rule.h"
#include "../graph/graph.h"
#include "../util/binary_stream.h"

std::atomic<int> ProductionRule::nextId{0};

vector<Graph*> copyAndRemoveSplices(vector<Graph*> startGraphs) {
    vector<Graph*> endGraphs;
    for (auto* startGraph : startGraphs) {
        auto* endGraph = startGraph->copy();
        endGraph->removeSplices();
        endGraphs.push_back(endGraph);
    }
    return endGraphs;
}

ProductionRule::ProductionRule(const vector<Graph*>& startGraphs) :
    ProductionRule(startGraphs, copyAndRemoveSplices(startGraphs)) {}

ProductionRule::ProductionRule(
    const vector<Graph*>& startGraphs,
    const vector<Graph*>& endGraphs
) : startGraphs(startGraphs),
    endGraphs(endGraphs),
    ground(false),
    id(nextId++) {}

ProductionRule::~ProductionRule() {
    for (auto graph : startGraphs) {
        delete graph;
    }
    for (auto graph : endGraphs) {
        delete graph;
    }
}

const vector<Graph*>& ProductionRule::getStartGraphs() const {
    return startGraphs;
}

const vector<Graph*>& ProductionRule::getEndGraphs() const {
    return endGraphs;
}

bool ProductionRule::isGround() const {
    return ground;
}

int ProductionRule::getId() const {
    return id;
}

ProductionRule* ProductionRule::binaryDeserialize(std::istream& in, Primitives* shape) {
    bool ground = bsRead<uint8_t>(in) != 0;
    int32_t n   = bsRead<int32_t>(in);
    vector<Graph*> startGraphs, endGraphs;
    startGraphs.reserve(n);
    endGraphs.reserve(n);
    for (int32_t i = 0; i < n; i++) {
        startGraphs.push_back(Graph::binaryDeserialize(in, shape));
        endGraphs.push_back(Graph::binaryDeserialize(in, shape));
    }
    auto* result = new ProductionRule(startGraphs, endGraphs);
    result->ground = ground;
    return result;
}

ProductionRule* ProductionRule::import(const Json& json, Primitives* shape) {
    vector<Graph*> startGraphs;
    for (size_t index = 0; index < json["n"].size(); ++index) {
        const auto& graphJson = json["n"][index];
        startGraphs.push_back(Graph::import(graphJson, shape));
    }

    size_t vertexSize = startGraphs[0]->getBVertices().size();
    size_t halfEdgeSize = startGraphs[0]->getBHalfEdges().size();
    size_t faceSize = startGraphs[0]->getBFaces().size();
    for (size_t i = 1; i < startGraphs.size(); i++) {
        if (startGraphs[i]->getBVertices().size() != vertexSize) {
            throw runtime_error("Boundary vertex mismatch");
        }
    }

    auto* result = new ProductionRule(startGraphs);
    result->ground = json["ground"];
    return result;
}
