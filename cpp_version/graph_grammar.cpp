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

NetworkHierarchy::NetworkHierarchy() {
    reset();
}

void NetworkHierarchy::reset() {
    generations.clear();
    nodeQueue.clear();
    emptyNet = nullptr;

    shape = nullptr;
    transitions.clear();
    starterTransitions.clear();
    groundTransitions.clear();
}

const std::vector<std::vector<Network*>>& NetworkHierarchy::getGenerations() const {
    return generations;
}

const std::vector<NetTransition*>& NetworkHierarchy::getTransitions() const {
    return transitions;
}

const std::vector<NetTransition*>& NetworkHierarchy::getStarterTransitions() const {
    return starterTransitions;
}

const std::vector<NetTransition*>& NetworkHierarchy::getGroundTransitions() const {
    return groundTransitions;
}

bool NetworkHierarchy::isGrounded() const {
    return grounded;
}

Transition NetworkHierarchy::getTransition() {
    NetTransition* transition = transitions.empty() ? nullptr : Util::pick<NetTransition*>(transitions);
    if (transition) {
        auto startNetworks = transition->getStartNetworks();
        auto endNetworks = transition->getEndNetworks();
        int n = (int)startNetworks.size();
        // Pick two unique indices
        int start = Util::randomInt(n);
        int end = Util::randomInt(n - 1);
        if (end >= start) {
            end++;
        }
        return { startNetworks[start], endNetworks[end], nullptr, false};
    }
    return {nullptr, nullptr, nullptr, false};
}

Transition NetworkHierarchy::getRemoveTransition() {
    bool grounded = starterTransitions.empty();
    if (grounded) {
        return {nullptr, nullptr, nullptr, false};
    }
    auto* transition = Util::pick(starterTransitions);
    if (transition) {
        auto startNetworks = transition->getStartNetworks();
        int n = (int)startNetworks.size();
        auto* endNet = emptyNet;
        auto* startNet = startNetworks[Util::randomInt(n - 1) + 1];
        return {startNet, endNet, nullptr, false};
    }
    return {nullptr, nullptr, nullptr, false};
}

Transition NetworkHierarchy::getStarterTransition(bool useGround) {
    bool grounded = (this->grounded && useGround) || starterTransitions.empty();
    auto& transitions = grounded ? groundTransitions : starterTransitions;
    
    auto* transition = Util::pick(transitions);
    if (transition) {
        auto startNetworks = transition->getStartNetworks();
        auto endNetworks = transition->getEndNetworks();
        int n = (int)endNetworks.size();
        auto* startNet = startNetworks[0];
        auto* endNet = endNetworks[Util::randomInt(n - 1) + 1];
        return {startNet, endNet, nullptr, transition->isGround()};
    }
    return {nullptr, nullptr, nullptr, false};
}

NetworkHierarchy* NetworkHierarchy::import(const Json& json) {
    auto* hierarchy = new NetworkHierarchy();
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
    hierarchy->emptyNet = Network::import(json["emptyNet"], hierarchy->shape);
    
    return hierarchy;
}

} // namespace ms 