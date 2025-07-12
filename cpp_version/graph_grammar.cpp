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
    emptyGraph = nullptr;
    primitives = nullptr;
    grounded = false;
}

bool GraphGrammar::isGrounded() const {
    return grounded && groundRules.size() > 0;
}

// Randomly pick one of the normal production rules.
Production GraphGrammar::getProduction() {
    ProductionRule* rule = rules.empty() ? nullptr : Util::pick<ProductionRule*>(rules);
    if (!rule) {
        return { nullptr, nullptr, nullptr, false };
    }
    auto startGraphs = rule->getStartGraphs();
    auto endGraphs = rule->getEndGraphs();
    int n = (int)startGraphs.size();

    // From the rule pick one start graph and one end graph that is different.
    int start = Util::randomInt(n);
    int end = Util::randomInt(n - 1);
    if (end >= start) {
        end++;
    }
    return { startGraphs[start], endGraphs[end], nullptr, false};
}

// Find a production that starts with an empty graph and adds a new graph.
// If useGround is true we take it from the ground rules.
Production GraphGrammar::getStarterProduction(bool useGround) {
    bool isGrounded = (grounded && useGround) || starterRules.empty();
    auto& rules = isGrounded ? groundRules : starterRules;

    auto* rule = Util::pick(rules);
    if (rule) {
        auto startGraphs = rule->getStartGraphs();
        auto endGraphs = rule->getEndGraphs();
        int n = (int)endGraphs.size();
        auto* startGraph = startGraphs[0];
        auto* endGraph = endGraphs[Util::randomInt(n - 1) + 1];
        return { startGraph, endGraph, nullptr, rule->isGround() };
    }
    return { nullptr, nullptr, nullptr, false };
}

// Find the opposite of getStarterProduction that is a production that starts with
// graph and replaces it with the empty graph.
Production GraphGrammar::getRemoveProduction() {
    if (starterRules.empty()) {
        return {nullptr, nullptr, nullptr, false};
    }
    auto* rule = Util::pick(starterRules);
    if (!rule) {
        return { nullptr, nullptr, nullptr, false };
    }
    auto startGraphs = rule->getStartGraphs();
    int n = (int)startGraphs.size();
    auto* endGraph = emptyGraph;
    auto* startGraph = startGraphs[Util::randomInt(n - 1) + 1];
    return {startGraph, endGraph, nullptr, false};
    
}

GraphGrammar* GraphGrammar::import(const Json& json) {
    auto* grammar = new GraphGrammar();
    grammar->primitives = Primitives::import(json["types"]);
    
    auto importRule = [&](const Json& transJson) {
        return ProductionRule::import(transJson, grammar->primitives);
    };
    
    for (const auto& transJson : json["rules"]) {
        grammar->rules.push_back(importRule(transJson));
    }
    for (const auto& transJson : json["starterRules"]) {
        grammar->starterRules.push_back(importRule(transJson));
    }
    for (const auto& transJson : json["groundRules"]) {
        grammar->groundRules.push_back(importRule(transJson));
    }
    
    grammar->grounded = json["grounded"];
    grammar->emptyGraph = Graph::import(json["emptyGraph"], grammar->primitives);

    return grammar;
}
