#include "pch.h"
#include "face_net.h"
#include "half_edge_net.h"
#include "network.h"
#include "../shape/vec3.h"

namespace ms {

FaceNet::FaceNet() 
    : outerComponent(nullptr)
    , type(nullptr)
    , network(nullptr) {
}

HalfEdgeNet* FaceNet::getOuterComponent() const {
    return outerComponent;
}

const std::vector<HalfEdgeNet*>& FaceNet::getInnerComponents() const {
    return innerComponents;
}

Network* FaceNet::getNetwork() const {
    return network;
}

FaceNet* FaceNet::connectNet(Network* network) {
    this->network = network;
    network->addFace(this);
    return this;
}

void FaceNet::connectOuter(const std::vector<HalfEdgeNet*>& halfEdges) {
    if (!halfEdges.empty()) {
        outerComponent = halfEdges[0];
        for (auto halfEdge : halfEdges) {
            halfEdge->setFace(this);
        }
    }
}

void FaceNet::makeInner(HalfEdgeNet* halfEdge) {
    innerComponents.push_back(halfEdge);
    outerComponent = nullptr;
}

void FaceNet::copyConnection(const FaceNet* copy) {
    Network* copyNet = copy->getNetwork();
    outerComponent = network->convertHalfEdge(copyNet, copy->getOuterComponent());
    
    innerComponents.clear();
    for (auto halfEdge : copy->getInnerComponents()) {
        innerComponents.push_back(network->convertHalfEdge(copyNet, halfEdge));
    }
}

void FaceNet::import(const Json & json) {
    auto halfEdges = network->getHalfEdges();
    if (halfEdges.size() == 0) {
        return;
    }
    outerComponent = network->getHalfEdges()[json["outerComponent"]];

    // innerComponents.clear();
    // for (const auto& halfEdge : json["innerComponents"]) {
    //    Vec3 dir = Vec3::import(halfEdge);
    //    // Create a lambda to mimic the getDir behavior
    //    innerComponents.push_back(new HalfEdgeNet(dir));
    // }
}

std::vector<HalfEdgeNet*> FaceNet::getConnectedHalfEdges(HalfEdgeNet* start) {
    std::vector<HalfEdgeNet*> result;
    HalfEdgeNet* current = start;
    do {
        result.push_back(current);
        current = current->getNext();
    } while (current && current != start);
    return result;
}

std::vector<HalfEdgeNet*> FaceNet::getOuterHalfEdges() const {
    if (outerComponent) {
        return getConnectedHalfEdges(outerComponent);
    }
    return std::vector<HalfEdgeNet*>();
}

std::vector<HalfEdgeNet*> FaceNet::getInnerHalfEdges() const {
    std::vector<HalfEdgeNet*> result;
    for (auto component : innerComponents) {
        auto connected = getConnectedHalfEdges(component);
        result.insert(result.end(), connected.begin(), connected.end());
    }
    return result;
}

std::vector<HalfEdgeNet*> FaceNet::getHalfEdges() const {
    auto result = getOuterHalfEdges();
    auto inner = getInnerHalfEdges();
    result.insert(result.end(), inner.begin(), inner.end());
    return result;
}

void FaceNet::replaceHalfEdge(HalfEdgeNet* a, HalfEdgeNet* b, bool force) {
    if ((outerComponent == a) || (force && outerComponent)) {
        outerComponent = b;
    }
    
    for (size_t i = 0; i < innerComponents.size(); i++) {
        if ((innerComponents[i] == a) || (force && innerComponents[i])) {
            innerComponents[i] = b;
        }
    }
    b->setFace(this);
}

bool FaceNet::isLoopy() const {
    return outerComponent->isLoopy();
}

bool FaceNet::inNetwork() const {
    auto faces = network->getFaces();
    return std::find(faces.begin(), faces.end(), this) != faces.end();
}

} // namespace ms 