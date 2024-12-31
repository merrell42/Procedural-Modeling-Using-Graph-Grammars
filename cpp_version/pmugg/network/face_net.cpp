#include "face_net.h"
#include "half_edge_net.h"
#include "network.h"
// #include "view.h"
#include "bound_net.h"
#include "half_edge_net.h"
#include "../shape/vec3.h"

namespace ms {

FaceNet::FaceNet() 
    : outerComponent(nullptr)
    , primal(nullptr)
    , network(nullptr) {
}

HalfEdgeNet* FaceNet::getOuterComponent() const {
    return outerComponent;
}

const std::vector<HalfEdgeNet*>& FaceNet::getInnerComponents() const {
    return innerComponents;
}

PrimalFace* FaceNet::getPrimal() const {
    return primal;
}

Network* FaceNet::getNetwork() const {
    return network;
}

void FaceNet::setPrimal(PrimalFace* primal) {
    this->primal = primal;
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

//Json FaceNet::export() const {
//    Json json;
//    json["outerComponent"] = network->halfEdgeIndex(getOuterComponent());
//    
//    std::vector<Json> innerComps;
//    for (auto halfEdge : getInnerComponents()) {
//        innerComps.push_back(halfEdge->getDir().export());
//    }
//    json["innerComponents"] = innerComps;
//    
//    return json;
//}

void FaceNet::import(const Json & json) {
    // Assuming network is already set and has a method getHalfEdges()
    //outerComponent = network->getHalfEdges()[json["outerComponent"]];

    //innerComponents.clear();  // Clear existing inner components
    //for (const auto& halfEdge : json["innerComponents"]) {
    //    Vec3 dir = Vec3::import(halfEdge);  // Assuming Vec3 has an import method
    //    // Create a lambda to mimic the getDir behavior
    //    innerComponents.push_back(new HalfEdgeNet(dir));  // Assuming HalfEdge can be constructed with a direction
    //}
}

std::vector<const HalfEdgeNet*> FaceNet::getConnectedHalfEdges(const HalfEdgeNet* start) {
    std::vector<const HalfEdgeNet*> result;
    const HalfEdgeNet* current = start;
    do {
        result.push_back(current);
        current = current->getNext();
    } while (current && current != start);
    return result;
}

std::vector<const HalfEdgeNet*> FaceNet::getOuterHalfEdges() const {
    if (outerComponent) {
        return getConnectedHalfEdges(outerComponent);
    }
    return std::vector<const HalfEdgeNet*>();
}

std::vector<const HalfEdgeNet*> FaceNet::getInnerHalfEdges() const {
    std::vector<const HalfEdgeNet*> result;
    for (auto component : innerComponents) {
        auto connected = getConnectedHalfEdges(component);
        result.insert(result.end(), connected.begin(), connected.end());
    }
    return result;
}

std::vector<const HalfEdgeNet*> FaceNet::getHalfEdges() const {
    auto result = getOuterHalfEdges();
    auto inner = getInnerHalfEdges();
    result.insert(result.end(), inner.begin(), inner.end());
    return result;
}

//void FaceNet::highlight(View* view) {
//    DrawOptions options;
//    options.halfEdges = getHalfEdges();
//    options.drawBackwards = false;  // TODO: Handle tempDrawBacks
//    network->highlight(view, options);
//}

//void FaceNet::getHalfEdgeSet() {
//    auto halfEdges = getHalfEdges();
//    if (!primal->interior) {
//        HalfEdgeSet result({}, {});
//        for (auto halfEdge : halfEdges) {
//            result.concat(halfEdge->getHalfEdgeSet());
//        }
//        return result;
//    } else {
//        return HalfEdgeSet(halfEdges);
//    }
//}

//void FaceNet::merge(FaceNet* faceB) {
//    auto bInner = faceB->getInnerComponents();
//    innerComponents.insert(innerComponents.end(), bInner.begin(), bInner.end());
//    
//    auto halfEdges = getConnectedHalfEdges(faceB->getOuterComponent());
//    for (auto inner : bInner) {
//        auto connected = getConnectedHalfEdges(inner);
//        halfEdges.insert(halfEdges.end(), connected.begin(), connected.end());
//    }
//    
//    for (auto halfEdge : halfEdges) {
//        halfEdge->setFace(this);
//    }
//    
//    BoundNet::mergeInto(this, faceB);
//}

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