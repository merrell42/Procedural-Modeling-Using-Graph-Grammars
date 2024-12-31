#include "primal_vertex.h"
#include "vertex_type.h"
#include "dual_object.h"
#include "bound_net.h"
#include "types.h"
#include "half_edge.h"
#include "edge.h"
#include "util.h"

namespace ms {

int PrimalVertex::nextId = 0;

PrimalVertex::PrimalVertex(VertexType* vertexType, DualObject* opt_connection)
    : type(vertexType)
    , connection(opt_connection)
    , id(nextId++) {}

DualObject* PrimalVertex::interiorEdge() const {
    if (interior.empty()) return nullptr;
    
    auto* vertex = interior[0];
    auto halfs = vertex->getHalfEdges();
    
    // Find first half-edge that has an edge
    auto it = std::find_if(halfs.begin(), halfs.end(),
        [](HalfEdge* half) { return half->getEdge() != nullptr; });
    
    return it != halfs.end() ? (*it)->getEdge() : nullptr;
}

void PrimalVertex::print() const {
    if (!interior.empty()) {
        interior[0]->print();
    }
}

PrimalObject* PrimalVertex::copy() const {
    return new PrimalVertex(*this);
}

void PrimalVertex::import(BoundNet* boundNet, Types* types, const Json& json) {
    // Get interior connection
    int interiorIndex = json["interior"].get<int>();
    if (interiorIndex >= 0) {
        auto* interiorB = boundNet->getInterior()->getVertices()[interiorIndex];
        BoundNet::connectInterior(this, interiorB);
    }

    // Get boundary connection
    int boundaryIndex = json["boundary"].get<int>();
    if (boundaryIndex >= 0) {
        auto* boundaryB = boundNet->getBoundary()->getVertices()[boundaryIndex];
        BoundNet::connectBoundary(this, boundaryB);
    }

    // Set type based on kind
    if (json["kind"] == "v") {
        type = types->vertexTypes[json["type"].get<int>()];
    } else if (json["kind"] == "e") {
        type = types->edgeTypes[json["type"].get<int>()];
    }
}

Json PrimalVertex::export() const {
    Json json;
    
    // Export connections
    auto* i = getInterior();
    auto* b = getBoundary();
    json["interior"] = i ? i->getNetwork()->vertexIndex(i) : -1;
    json["boundary"] = b ? b->getNetwork()->vertexIndex(b) : -1;
    
    // Export type information
    if (type) {
        auto* boundNet = type->getNetwork()->getBoundNet();
        auto* types = boundNet->getTypes();
        
        // Determine if this is a vertex or edge type
        auto vertexIt = std::find(types->vertexTypes.begin(), 
                                types->vertexTypes.end(), type);
        if (vertexIt != types->vertexTypes.end()) {
            json["kind"] = "v";
            json["type"] = vertexIt - types->vertexTypes.begin();
        } else {
            json["kind"] = "e";
            auto edgeIt = std::find(types->edgeTypes.begin(), 
                                  types->edgeTypes.end(), type);
            json["type"] = edgeIt - types->edgeTypes.begin();
        }
    }
    
    return json;
}

} // namespace ms 