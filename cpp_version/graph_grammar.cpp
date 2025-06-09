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

GraphGrammar::GraphGrammar() {
    reset();
}

void GraphGrammar::reset() {
    generations.clear();
    nodeQueue.clear();
    emptyGraph = nullptr;

    shape = nullptr;
    rules.clear();
    starterRules.clear();
    groundRules.clear();
}

const std::vector<std::vector<Graph*>>& GraphGrammar::getGenerations() const {
    return generations;
}

const std::vector<ProductionRule*>& GraphGrammar::getRules() const {
    return rules;
}

const std::vector<ProductionRule*>& GraphGrammar::getStarterRules() const {
    return starterRules;
}

const std::vector<ProductionRule*>& GraphGrammar::getGroundRules() const {
    return groundRules;
}

bool GraphGrammar::isGrounded() const {
    return grounded;
}

Production GraphGrammar::getProduction() {
    ProductionRule* rule = rules.empty() ? nullptr : Util::pick<ProductionRule*>(rules);
    if (rule) {
        auto startGraphs = rule->getStartGraphs();
        auto endGraphs = rule->getEndGraphs();
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
    bool grounded = starterRules.empty();
    if (grounded) {
        return {nullptr, nullptr, nullptr, false};
    }
    auto* rule = Util::pick(starterRules);
    if (rule) {
        auto startGraphs = rule->getStartGraphs();
        int n = (int)startGraphs.size();
        auto* endGraph = emptyGraph;
        auto* startGraph = startGraphs[Util::randomInt(n - 1) + 1];
        return {startGraph, endGraph, nullptr, false};
    }
    return {nullptr, nullptr, nullptr, false};
}

Production GraphGrammar::getStarterProduction(bool useGround) {
    bool grounded = (this->grounded && useGround) || starterRules.empty();
    auto& rules = grounded ? groundRules : starterRules;
    
    auto* rule = Util::pick(rules);
    if (rule) {
        auto startGraphs = rule->getStartGraphs();
        auto endGraphs = rule->getEndGraphs();
        int n = (int)endGraphs.size();
        auto* startGraph = startGraphs[0];
        auto* endGraph = endGraphs[Util::randomInt(n - 1) + 1];
        return {startGraph, endGraph, nullptr, rule->isGround()};
    }
    return {nullptr, nullptr, nullptr, false};
}

GraphGrammar* GraphGrammar::import(const Json& json) {
    auto* hierarchy = new GraphGrammar();
    hierarchy->shape = Primitives::import(json["types"]);
    
    auto importRule = [&](const Json& transJson) {
        return ProductionRule::import(transJson, hierarchy->shape);
    };
    
    for (const auto& transJson : json["transitions"]) {
        hierarchy->rules.push_back(importRule(transJson));
    }
    for (const auto& transJson : json["starterTransitions"]) {
        hierarchy->starterRules.push_back(importRule(transJson));
    }
    for (const auto& transJson : json["groundTransitions"]) {
        hierarchy->groundRules.push_back(importRule(transJson));
    }
    
    hierarchy->grounded = json["grounded"];
    hierarchy->emptyGraph = Graph::import(json["emptyNet"], hierarchy->shape);
    
    return hierarchy;
}

