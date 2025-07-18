#include "pch.h"
#include "production_rule.h"
#include "../graph/graph.h"

int ProductionRule::nextId = 0;

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

ProductionRule* ProductionRule::import(const Json& json, Primitives* shape) {
    vector<Graph*> startGraphs;
    vector<Graph*> endGraphs;
    for (size_t index = 0; index < json["n"].size(); ++index) {
        const auto& graphJson = json["n"][index];
        startGraphs.push_back(Graph::import(graphJson, shape));

        // End graphs are the same as start graphs except the splices are removed.
        // This could be done by copying startGraph rather than importing it again.
        const auto endGraph = Graph::import(graphJson, shape);
        endGraph->removeSplices();
        endGraphs.push_back(endGraph);
    }

    size_t vertexSize = startGraphs[0]->getBVertices().size();
    size_t halfEdgeSize = startGraphs[0]->getBHalfEdges().size();
    size_t faceSize = startGraphs[0]->getBFaces().size();
    for (size_t i = 1; i < startGraphs.size(); i++) {
        if (startGraphs[i]->getBVertices().size() != vertexSize) {
            throw runtime_error("Boundary vertex mismatch");
        }
    }

    auto* result = new ProductionRule(startGraphs, endGraphs);
    result->ground = json["ground"];
    return result;
}
