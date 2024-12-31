#include "primal_edge.h"
#include "../shapes3D/edge_type3d.h"
#include "bound_net.h"
#include "../shapes3D/shape3d.h"
#include "../util/util.h"

namespace ms {

int PrimalEdge::nextId = 0;

PrimalEdge::PrimalEdge(EdgeType3D* edgeType)
    : type(edgeType)
    , id(nextId++) {}

std::string PrimalEdge::boundaryString() const {
    return type->boundaryString();
}

//void PrimalEdge::print() const {
//    if (!interior.empty()) {
//        interior[0]->print();
//    }
//}

PrimalEdge* PrimalEdge::copy() const {
    return new PrimalEdge(type);
}

void PrimalEdge::import(BoundNet* boundNet, Shape3D* shape, const Json& json) {
    // Get interior connection
    int interiorIndex = json["interior"].get<int>();
    if (interiorIndex >= 0) {
        auto* interiorB = boundNet->getInterior()->getEdges()[interiorIndex];
        BoundNet::connectInterior(this, interiorB);
    }

    // Set type
    type = shape->edgeTypes[json["type"].get<int>()];
}

//Json PrimalEdge::export() const {
//    Json json;
//    
//    // Export interior connection
//    auto* i = getInterior();
//    auto* network = i ? i->getNetwork() : nullptr;
//    json["interior"] = i ? network->edgeIndex(i) : -1;
//    
//    // Export type
//    json["type"] = type->getNetwork()->getBoundNet()->getTypes()->edgeTypes.indexOf(type);
//    
//    return json;
//}

} // namespace ms 