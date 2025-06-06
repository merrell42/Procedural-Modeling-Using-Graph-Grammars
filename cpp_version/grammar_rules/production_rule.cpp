#include "pch.h"
#include "production_rule.h"
#include "../graph/graph.h"

int NetTransition::nextId = 0;

NetTransition::NetTransition(
    const vector<Graph*>& startGraphs,
    const vector<Graph*>& endGraphs
) : startGraphs(startGraphs),
    endGraphs(endGraphs),
    ground(false),
    id(nextId++) {}

NetTransition* NetTransition::import(const Json& json, Primitives* shape) {
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

    auto* result = new NetTransition(startGraphs, endGraphs);
    result->ground = json["ground"];
    return result;
}

