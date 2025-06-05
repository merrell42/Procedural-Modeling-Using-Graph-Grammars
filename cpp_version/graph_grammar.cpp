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
    emptyNet = nullptr;

    shape = nullptr;
    transitions.clear();
    starterTransitions.clear();
    groundTransitions.clear();
}

const std::vector<std::vector<Graph*>>& GraphGrammar::getGenerations() const {
    return generations;
}

const std::vector<NetTransition*>& GraphGrammar::getTransitions() const {
    return transitions;
}

const std::vector<NetTransition*>& GraphGrammar::getStarterTransitions() const {
    return starterTransitions;
}

const std::vector<NetTransition*>& GraphGrammar::getGroundTransitions() const {
    return groundTransitions;
}

bool GraphGrammar::isGrounded() const {
    return grounded;
}

Transition GraphGrammar::getTransition() {
    NetTransition* transition = transitions.empty() ? nullptr : Util::pick<NetTransition*>(transitions);
    if (transition) {
        auto startGraphs = transition->getStartGraphs();
        auto endGraphs = transition->getEndGraphs();
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

Transition GraphGrammar::getRemoveTransition() {
    bool grounded = starterTransitions.empty();
    if (grounded) {
        return {nullptr, nullptr, nullptr, false};
    }
    auto* transition = Util::pick(starterTransitions);
    if (transition) {
        auto startGraphs = transition->getStartGraphs();
        int n = (int)startGraphs.size();
        auto* endNet = emptyNet;
        auto* startNet = startGraphs[Util::randomInt(n - 1) + 1];
        return {startNet, endNet, nullptr, false};
    }
    return {nullptr, nullptr, nullptr, false};
}

Transition GraphGrammar::getStarterTransition(bool useGround) {
    bool grounded = (this->grounded && useGround) || starterTransitions.empty();
    auto& transitions = grounded ? groundTransitions : starterTransitions;
    
    auto* transition = Util::pick(transitions);
    if (transition) {
        auto startGraphs = transition->getStartGraphs();
        auto endGraphs = transition->getEndGraphs();
        int n = (int)endGraphs.size();
        auto* startNet = startGraphs[0];
        auto* endNet = endGraphs[Util::randomInt(n - 1) + 1];
        return {startNet, endNet, nullptr, transition->isGround()};
    }
    return {nullptr, nullptr, nullptr, false};
}

GraphGrammar* GraphGrammar::import(const Json& json) {
    auto* hierarchy = new GraphGrammar();
    hierarchy->shape = Shape3D::import(json["types"]);
    
    auto importTransition = [&](const Json& transJson) {
        return NetTransition::import(transJson, hierarchy->shape);
    };
    
    for (const auto& transJson : json["transitions"]) {
        hierarchy->transitions.push_back(importTransition(transJson));
    }
    for (const auto& transJson : json["starterTransitions"]) {
        hierarchy->starterTransitions.push_back(importTransition(transJson));
    }
    for (const auto& transJson : json["groundTransitions"]) {
        hierarchy->groundTransitions.push_back(importTransition(transJson));
    }
    
    hierarchy->grounded = json["grounded"];
    hierarchy->emptyNet = Graph::import(json["emptyNet"], hierarchy->shape);
    
    return hierarchy;
}

} // namespace ms 