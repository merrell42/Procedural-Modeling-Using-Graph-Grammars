#include "pch.h"
#include "graph_grammar.h"
#include "graph/graph.h"
#include "grammar_rules/production_rule.h"
#include "primitives/edge_type.h"
#include "primitives/face_type.h"
#include "primitives/vertex_type.h"
#include "settings.h"
#include "util/util.h"
#include <algorithm>

namespace ms {

GraphGrammar::GraphGrammar() {
    reset();
}

void GraphGrammar::reset() {
    generations.clear();
    nodeQueue.clear();
    emptyGraph = nullptr;

    shape = nullptr;
    productions.clear();
    starterProductions.clear();
    groundProductions.clear();
}

const std::vector<std::vector<Graph*>>& GraphGrammar::getGenerations() const {
    return generations;
}

const std::vector<ProductionRule*>& GraphGrammar::getProductions() const {
    return productions;
}

const std::vector<ProductionRule*>& GraphGrammar::getStarterProductions() const {
    return starterProductions;
}

const std::vector<ProductionRule*>& GraphGrammar::getGroundProductions() const {
    return groundProductions;
}

bool GraphGrammar::isGrounded() const {
    return grounded;
}

Production GraphGrammar::getProduction() {
    ProductionRule* production = productions.empty() ? nullptr : Util::pick<ProductionRule*>(productions);
    if (production) {
        auto startGraphs = production->getStartGraphs();
        auto endGraphs = production->getEndGraphs();
        int n = (int)startGraphs.size();
        // Pick two unique indices
        int start = Util::randomInt(n);
        int end = Util::randomInt(n - 1);
        if (end >= start) {
            end++;
        }
        return { startGraphs[start], endGraphs[end], nullptr, false};
    }
    return {nullptr, nullptr, nullptr, false};
}

Production GraphGrammar::getRemoveProduction() {
    bool grounded = starterProductions.empty();
    if (grounded) {
        return {nullptr, nullptr, nullptr, false};
    }
    auto* production = Util::pick(starterProductions);
    if (production) {
        auto startGraphs = production->getStartGraphs();
        int n = (int)startGraphs.size();
        auto* endGraph = emptyGraph;
        auto* startGraph = startGraphs[Util::randomInt(n - 1) + 1];
        return {startGraph, endGraph, nullptr, false};
    }
    return {nullptr, nullptr, nullptr, false};
}

Production GraphGrammar::getStarterProduction(bool useGround) {
    bool grounded = (this->grounded && useGround) || starterProductions.empty();
    auto& productions = grounded ? groundProductions : starterProductions;
    
    auto* production = Util::pick(productions);
    if (production) {
        auto startGraphs = production->getStartGraphs();
        auto endGraphs = production->getEndGraphs();
        int n = (int)endGraphs.size();
        auto* startGraph = startGraphs[0];
        auto* endGraph = endGraphs[Util::randomInt(n - 1) + 1];
        return {startGraph, endGraph, nullptr, production->isGround()};
    }
    return {nullptr, nullptr, nullptr, false};
}

GraphGrammar* GraphGrammar::import(const Json& json) {
    auto* hierarchy = new GraphGrammar();
    hierarchy->shape = Primitives::import(json["types"]);
    
    auto importProduction = [&](const Json& transJson) {
        return ProductionRule::import(transJson, hierarchy->shape);
    };
    
    // TODO: Rename transitions in the JSON.
    for (const auto& transJson : json["transitions"]) {
        hierarchy->productions.push_back(importProduction(transJson));
    }
    for (const auto& transJson : json["starterTransitions"]) {
        hierarchy->starterProductions.push_back(importProduction(transJson));
    }
    for (const auto& transJson : json["groundTransitions"]) {
        hierarchy->groundProductions.push_back(importProduction(transJson));
    }
    
    hierarchy->grounded = json["grounded"];
    // TODO: Rename emptyMap in the JSON.
    hierarchy->emptyGraph = Graph::import(json["emptyNet"], hierarchy->shape);
    
    return hierarchy;
}

} // namespace ms 