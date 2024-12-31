#include "primal_volume.h"
#include "dual_object.h"
#include "bound_net.h"
#include "types.h"
#include "half_edge.h"
#include "face.h"
#include "util.h"

namespace ms {

int PrimalVolume::nextId = 0;

PrimalVolume::PrimalVolume(int volumeType)
    : type(volumeType)
    , id(nextId++) {}

std::string PrimalVolume::boundaryString() const {
    if (!getBoundary()) return "";

    std::string result;
    auto* face = getBoundary();
    auto outerHalfs = face->getOuterHalfEdges();

    for (auto* half : outerHalfs) {
        if (auto* edge = half->getEdge()) {
            if (auto* primal = edge->getPrimal()) {
                result += primal->boundaryString();
            }
        }
    }

    return result;
}

void PrimalVolume::print() const {
    std::cout << boundaryString() << std::endl;
    if (getBoundary()) {
        getBoundary()->print();
    }
}

PrimalVolume* PrimalVolume::copy() const {
    return new PrimalVolume(type);
}

void PrimalVolume::import(BoundNet* boundNet, Types* types, const Json& json) {
    // Get boundary connection
    int boundaryIndex = json["boundary"].get<int>();
    if (boundaryIndex >= 0) {
        auto* boundaryB = boundNet->getBoundary()->getFaces()[boundaryIndex];
        BoundNet::connectBoundary(this, boundaryB);
    }

    // Set type
    type = json["type"].get<int>();
}

Json PrimalVolume::export() const {
    Json json;
    
    // Export boundary connection
    auto* b = getBoundary();
    json["boundary"] = b ? b->getNetwork()->faceIndex(b) : -1;
    
    // Export type
    json["type"] = type;
    
    return json;
}

} // namespace ms 