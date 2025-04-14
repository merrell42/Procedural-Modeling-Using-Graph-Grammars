#include "pch.h"
#include "edge_net.h"
#include "network.h"
#include "half_edge_net.h"
// #include "view.h"
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

//PrimalEdge* EdgeNet::getPrimal() const {
//    return primal;
//}

Network* EdgeNet::getNetwork() const {
    return network;
}

int EdgeNet::getId() const {
    return id;
}

//void EdgeNet::setPrimal(PrimalEdge* p) {
//    primal = p;
//}

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

//void EdgeNet::highlight(View* view) {
//    std::vector<HalfEdgeNet*> allHalfEdges;
//    for (const auto& halfArray : halfEdges) {
//        allHalfEdges.insert(allHalfEdges.end(), halfArray.begin(), halfArray.end());
//    }
//    
//    auto options = HalfEdgeSet::create(allHalfEdges);
//    network->highlight(view, options);
//}
//
//void EdgeNet::print() const {
//    auto* interior = primal->getInterior();
//    if (interior != this) {
//        std::cout << "Boundary Edge" << std::endl;
//        interior->print();
//    } else {
//        ms::highlight(this);
//    }
//}

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
            halfA->connectVertex(halfB->getVertex(), halfA->getVertexIndex());
        }
        
        auto* face = halfB->getFace();
        face->replaceHalfEdge(halfB, halfA, false);
        interior->removeHalfEdge(halfB);
    }
    interior->removeEdge(edgeB);
}

//Json EdgeNet::export() const {
//    Json json;
//    json["halfEdges"] = Json::array();
//    
//    for (const auto& halfArray : halfEdges) {
//        Json arrayJson = Json::array();
//        for (auto* half : halfArray) {
//            arrayJson.push_back(network->halfEdgeIndex(half));
//        }
//        json["halfEdges"].push_back(arrayJson);
//    }
//    
//    return json;
//}

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