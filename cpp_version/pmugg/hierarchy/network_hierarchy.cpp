#include "pch.h"
#include "network_hierarchy.h"
#include "../network/network.h"
#include "../network/net_transition.h"
#include "../shapes3D/edge_type3d.h"
#include "../shapes3D/face_type3d.h"
#include "../guidelines/vertex_type.h"
#include "../grid/settings.h"
#include "../util/util.h"
#include <algorithm>

namespace ms {

NetworkHierarchy::NetworkHierarchy() {
    reset();
}

void NetworkHierarchy::reset() {
    generations.clear();
    nodeQueue.clear();
    // edgeOptions.clear();
    // gluingOptions.clear();
    emptyNet = nullptr;

    shape = nullptr;
    transitions.clear();
    starterTransitions.clear();
    groundTransitions.clear();
    // grounded = globalSettings["Grounded"];
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

Network* NetworkHierarchy::getEmptyNet() const {
    return emptyNet;
}

bool NetworkHierarchy::isGrounded() const {
    return grounded;
}

Transition NetworkHierarchy::getTransition() {
    // TODO: Remove this after debugging.
    /* if (transitions.size() >= 2) {
        transitions.pop_back();
    } */

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

//void NetworkHierarchy::partialGenerate(const std::vector<Network*>& transitionNets,
//                                     NetworkSet& networks) {
//    // Implementation similar to JS version
//    // ... (about 50 lines of network generation code)
//}

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

//NetworkHierarchy* NetworkHierarchy::partialImport(const Json& json, const Json& decoration) {
//    auto* hierarchy = new NetworkHierarchy();
//    auto types = std::make_unique<Types>();
//
//    // Import face types
//    for (const auto& faceJson : json["faceTypes"]) {
//        auto* faceType = FaceType3D::partialImport(faceJson);
//        types->faceTypes[faceType->getId()] = faceType;
//    }
//
//    // Import edge types
//    for (const auto& edgeJson : json["edgeTypes"]) {
//        auto* edgeType = EdgeType3D::partialImport(edgeJson, types->faceTypes);
//        types->edgeTypes[edgeType->getId()] = edgeType;
//    }
//
//    // Import vertex types and apply decorations
//    auto vertexTypes = json["vertexTypes"].get<std::vector<Json>>();
//    for (const auto& vertexJson : vertexTypes) {
//        auto* vertexType = VertexType::partialImport(vertexJson, types->edgeTypes);
//        types->vertexTypes.push_back(vertexType);
//    }
//
//    if (!decoration.empty()) {
//        auto vDecorations = makeArray(decoration["decoration"]["vDecoration"]);
//        for (auto* vertexType : types->vertexTypes) {
//            auto id = vertexType->getId();
//            for (const auto& vDecoration : vDecorations) {
//                auto decorTypes = vDecoration["@attributes"]["types"];
//                if (activeAction(decorTypes, id)) {
//                    vertexType->getDecoration()->importXml(vDecoration);
//                }
//            }
//        }
//
//        auto eDecorations = makeArray(decoration["decoration"]["edgeBrush"]);
//        for (auto& [_, edgeType] : types->edgeTypes) {
//            auto id = edgeType->getIdNum();
//            for (const auto& eDecoration : eDecorations) {
//                auto decorTypes = eDecoration["@attributes"]["types"];
//                if (activeAction(decorTypes, id)) {
//                    edgeType->getBrush()->importXml(eDecoration);
//                }
//            }
//        }
//    }
//
//    // Create networks
//    NetworkSet networks;
//    networks.face = Util::transform(types->faceTypes, NetworkFactory::createFacePrimitive);
//    networks.edge = Util::transform(types->edgeTypes, NetworkFactory::createEdgePrimitive);
//    networks.vertex = Util::transform(types->vertexTypes, NetworkFactory::createVertexPrimitive);
//    networks.edgeTypes = Util::values(types->edgeTypes);
//
//    // Process matches
//    std::vector<Network*> transitionNets;
//    for (const auto& match : json["matches"]) {
//        if (auto* glued = glueMatch(match, networks.vertex, connectionOrder)) {
//            transitionNets.push_back(glued);
//        }
//    }
//
//    hierarchy->partialGenerate(transitionNets, networks);
//
//    // Filter transitions
//    hierarchy->starterTransitions = Util::filter(hierarchy->starterTransitions,
//        [](NetTransition* t) {
//            return t->getNetworks()[0]->getInterior().faces.size() > 0;
//        });
//
//    // Set ground transitions
//    auto faceTypes = Util::values(types->faceTypes);
//    std::sort(faceTypes.begin(), faceTypes.end(),
//        [](FaceType* a, FaceType* b) {
//            return std::stoi(a->getId()) < std::stoi(b->getId());
//        });
//
//    auto* groundType = Util::find(faceTypes,
//        [](FaceType* type) { return type->getNormal().z > 0.999; });
//
//    if (!groundType) {
//        throw std::runtime_error("No ground type found.");
//    }
//
//    hierarchy->groundTransitions = {TestRunner::getGroundTransition(groundType)};
//
//    return hierarchy;
//}

//Network* NetworkHierarchy::glueMatch(const Match& match,
//                                   const std::vector<Network*>& vertexNetworks,
//                                   const std::vector<std::vector<int>>& connectionOrder) {
//    // Implementation similar to JS version
//    // ... (about 100 lines of gluing code)
//    return nullptr; // Placeholder
//}

} // namespace ms 