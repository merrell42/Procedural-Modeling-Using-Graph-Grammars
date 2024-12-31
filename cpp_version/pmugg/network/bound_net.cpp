#include "bound_net.h"
#include "network.h"
#include "primal_vertex.h"
#include "primal_edge.h"
#include "primal_face.h"
#include "primal_volume.h"
#include "../shapes3D/shape3d.h"
#include "../shapes3D/edge_type3d.h"
// #include "view.h"
#include "../util/timer.h"
#include "../util/util.h"
#include "face_net.h"
#include <algorithm>

namespace ms {

int BoundNet::nextId = 0;

BoundNet::BoundNet(Network* interior, Network* boundary, Shape3D* shape3D)
    : interior(interior)
    , boundary(boundary)
    , shape3D(shape3D)
    , id(nextId++)
    , cachedConnectors() {
    
    interior->setBoundNet(this);
    boundary->setBoundNet(this);
}

Network* BoundNet::getBoundary() const {
    return boundary;
}

Network* BoundNet::getInterior() const {
    return interior;
}

Shape3D* BoundNet::getShape3D() const {
    return shape3D;
}

int BoundNet::getId() const {
    return id;
}

std::vector<void*> BoundNet::getConnectors() {
    std::vector<void*> emptylist;
    return emptylist;
    //if (cachedConnectors.empty()) {
    //    auto faces = boundary->getFaces();
    //    for (auto* face : faces) {
    //        if (!face) continue;
    //        
    //        auto outerHalfs = face->getOuterHalfEdges();
    //        for (auto* half : outerHalfs) {
    //            if (half->getNext() == half && !half->getVertex()) {
    //                // This is an outer face with no connectors
    //                continue;
    //            }
    //            auto* connector = half->boundaryToInterior()->getVertex()->getPrimal();
    //            if (std::find(cachedConnectors.begin(), cachedConnectors.end(), connector) 
    //                == cachedConnectors.end()) {
    //                cachedConnectors.push_back(connector);
    //            }
    //        }
    //    }
    //}
    //return cachedConnectors;
}

BoundNet* BoundNet::copy() const {
    // Timer::start("Copy Net");
    
    // Copy the two networks
    auto* copyI = interior->copy();
    auto* copyB = boundary->copy();
    auto* resultB = new BoundNet(copyI, copyB, shape3D);
    
    // Create a copy of primalA with the given interior and boundary
    auto createPrimal = [](void* primalA, void* interiorB, void* boundaryB) {
        auto* primalB = static_cast<PrimalObject*>(primalA)->copy();
        connectBoundary(primalB, boundaryB);
        connectInterior(primalB, interiorB);
    };

    // Copy the primal vertices, edges, faces, and volumes
    for (auto* objA : interior->getVertices()) {
        PrimalVertex* primalA = objA->getPrimal();
        auto* interiorB = resultB->interior->convertVertex(interior, primalA->getInterior());
        auto* boundaryB = resultB->boundary->convertVertex(boundary, primalA->getBoundary());
        createPrimal(primalA, interiorB, boundaryB);
    }

    for (auto* objA : interior->getEdges()) {
        PrimalEdge* primalA = objA->getPrimal();
        auto* interiorB = resultB->interior->convertEdge(interior, primalA->getInterior());
        createPrimal(primalA, interiorB, nullptr);
    }

    for (auto* objA : interior->getFaces()) {
        PrimalFace* primalA = objA->getPrimal();
        auto* interiorB = resultB->interior->convertFace(interior, primalA->getInterior());
        auto* boundaryB = resultB->boundary->convertEdge(boundary, primalA->getBoundary());
        createPrimal(primalA, interiorB, boundaryB);
    }

    /*for (auto* objA : boundary->getFaces()) {
        PrimalVolume* primalA = objA->getPrimal();
        auto* boundaryB = resultB->boundary->convertFace(boundary, primalA->getBoundary());
        createPrimal(primalA, nullptr, boundaryB);
    }*/

    // Timer::stop("Copy Net");
    return resultB;
}

//BoundNet* BoundNet::removeSplices() const {
//    bool hasSplices = std::any_of(interior->getHalfEdges().begin(),
//                                 interior->getHalfEdges().end(),
//                                 [](HalfEdge* half) { return half->isSpliced(); });
//    if (!hasSplices) {
//        return const_cast<BoundNet*>(this);
//    }
//
//    auto* result = copy();
//    // Cache connectors to avoid infinite loop in getConnectors
//    result->getConnectors();
//    
//    auto* interior = result->getInterior();
//    for (auto* half : interior->getHalfEdges()) {
//        auto* next = half->getNext();
//        if (!half->isSpliced() && next && next->isSpliced()) {
//            auto* newNext = next->getTwin()->getNext();
//            half->getEdge()->merge(newNext->getEdge(), half->getForward());
//        }
//    }
//
//    for (auto* half : interior->getHalfEdges()) {
//        if (half->isSpliced()) {
//            interior->removeHalfEdge(half);
//            auto* vertex = half->getVertex();
//            if (vertex->inNetwork()) {
//                interior->removeVertex(vertex);
//            }
//            auto* edge = half->getEdge();
//            if (edge->inNetwork()) {
//                interior->removeEdge(edge);
//            }
//        }
//    }
//
//    return result;
//}

void BoundNet::recomputeTurns() {
    for (auto* face : interior->getFaces()) {
        face->getPrimal()->computeTurns();
    }
}

std::vector<FaceNet*> BoundNet::getOuterFaces() const {
    auto faces = interior->getFaces();
    std::vector<FaceNet*> outerFaces;
    for (size_t i = 0; i < faces.size(); ++i) {
        auto face = faces[i];
        if (face->getPrimal()->getTurns() == 1 && face->isLoopy()) {
            outerFaces.push_back(face);
        }
    }
    return outerFaces;
}

//void BoundNet::highlight(View* view, const DrawOptions& options) {
//    interior->highlight(view, options);
//}
//
//void BoundNet::draw(View* view, const DrawOptions& options) {
//    interior->draw(view, options);
//}
//
//void BoundNet::print() const {
//    ms::highlight(interior);
//}

//void BoundNet::connectBoundary(void* primal, void* boundary) {
//    if (!boundary) return;
//    auto* p = static_cast<PrimalObject*>(primal);
//    p->boundary.push_back(boundary);
//    if (boundary) {
//        static_cast<DualObject*>(boundary)->setPrimal(p);
//    }
//}

void BoundNet::connectInterior(void* primal, void* interior) {
    if (!interior) return;
    auto* p = static_cast<PrimalObject*>(primal);
    p->interior.push_back(interior);
    if (interior) {
        static_cast<DualObject*>(interior)->setPrimal(p);
    }
}

BoundNet* BoundNet::import(const Json& json, Shape3D* shape3D) {
    auto* boundNet = new BoundNet(
        Network::import(json["interior"]),
        Network::import(json["boundary"]),
        shape3D
    );

    // Import the primal vertices, edges, faces, and volumes
    for (const auto& vertexJson : json["vertices"]) {
        auto* vertex = new PrimalVertex();
        vertex->import(boundNet, shape3D, vertexJson);
    }

    for (const auto& edgeJson : json["edges"]) {
        auto* edge = new PrimalEdge();
        edge->import(boundNet, shape3D, edgeJson);
    }

    for (const auto& faceJson : json["faces"]) {
        auto* face = new PrimalFace();
        face->import(boundNet, shape3D, faceJson);
    }

    for (const auto& volumeJson : json["volumes"]) {
        auto* volume = new PrimalVolume();
        volume->import(boundNet, shape3D, volumeJson);
    }

    return boundNet;
}

} // namespace ms 