#include "pch.h"
#include "graph.h"
#include "graph_edge.h"
#include "graph_half_edge.h"
#include "../util/util.h"

namespace ms {

int EdgeNet::nextId = 0;

EdgeNet::EdgeNet()
    : type(nullptr)
    , network(nullptr)
    , id(nextId++) {}

const std::vector<std::vector<HalfEdgeNet*>>& EdgeNet::getHalfEdges() const {
    return halfEdges;
}

Network* EdgeNet::getNetwork() const {
    return network;
}

int EdgeNet::getId() const {
    return id;
}

EdgeNet* EdgeNet::connectNet(Network* net) {
    network = net;
    network->addEdge(this);
    return this;
}

void EdgeNet::addHalfEdge(HalfEdgeNet* halfEdge, int index) {
    if (index >= halfEdges.size()) {
        halfEdges.resize(index + 1);
    }
    halfEdges[index].push_back(halfEdge);
}

void EdgeNet::removeHalfEdge(HalfEdgeNet* halfEdge, int index) {
    if (index < halfEdges.size()) {
        halfEdges.erase(halfEdges.begin() + index);
    }
}

void EdgeNet::copyConnection(const EdgeNet* copy) {
    auto* copyNet = copy->getNetwork();
    auto copyHalfEdges = copy->getHalfEdges();
    
    halfEdges.clear();
    halfEdges.resize(copyHalfEdges.size());
    
    for (size_t i = 0; i < copyHalfEdges.size(); i++) {
        for (auto* half : copyHalfEdges[i]) {
            halfEdges[i].push_back(network->convertHalfEdge(copyNet, half));
        }
    }
}

bool EdgeNet::inNetwork() const {
    auto vec = network->getEdges();
    return std::find(vec.begin(), vec.end(), this) != vec.end();
}

void EdgeNet::merge(EdgeNet* edgeB, bool mergeForward) {
    auto* interior = network;

    auto halfsB = edgeB->getHalfEdges();
    for (size_t edgeIndex = 0; edgeIndex < halfEdges.size(); edgeIndex++) {
        auto* halfA = halfEdges[edgeIndex][0];
        auto* halfB = halfsB[edgeIndex][0];
        auto forward = halfA->getForward();
        
        if (!(forward ^ mergeForward)) {
            auto* nextB = halfB->getNext();
            halfA->disconnectHalfEdge();
            halfB->disconnectHalfEdge();
            halfA->connectHalfEdge(nextB);
        } else {
            auto* prevB = halfB->getPrev();
            if (prevB) {
                prevB->disconnectHalfEdge();
                halfB->disconnectHalfEdge();
                prevB->connectHalfEdge(halfA);
            }
            halfA->connectVertex(halfB->getVertex(), halfB->getVertexIndex());
        }
        
        auto* face = halfB->getFace();
        face->replaceHalfEdge(halfB, halfA, false);
        interior->removeHalfEdge(halfB);
    }
    interior->removeEdge(edgeB);
}

void EdgeNet::import(const Json& json) {
    halfEdges.clear();
    auto& networkHalfEdges = network->getHalfEdges();
    
    if (json.contains("halfEdges")) {
        for (const auto& arrayJson : json["halfEdges"]) {
            std::vector<HalfEdgeNet*> halfArray;
            for (const auto& index : arrayJson) {
                halfArray.push_back(networkHalfEdges[index.get<int>()]);
            }
            halfEdges.push_back(halfArray);
        }
    }
}

} // namespace ms 