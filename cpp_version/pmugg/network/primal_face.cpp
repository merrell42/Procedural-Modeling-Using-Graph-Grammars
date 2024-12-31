#include "primal_face.h"
#include "face_type.h"
#include "dual_object.h"
#include "bound_net.h"
#include "types.h"
#include "half_edge.h"
#include "face_net.h"
#include "util.h"
#include <cmath>

namespace ms {

int PrimalFace::nextId = 0;

PrimalFace::PrimalFace(FaceType* faceType, int numTurns, bool isWildcard)
    : type(faceType)
    , turns(numTurns)
    , wildcard(isWildcard)
    , id(nextId++) {}

void PrimalFace::computeTurns() {
    auto* halfEdge = getInterior() ? 
        static_cast<FaceNet*>(getInterior())->getOuterComponent() : nullptr;
    
    if (!halfEdge) {
        turns = 0;
        return;
    }

    // Get connected half edges
    std::vector<HalfEdge*> faceHalfs;
    auto* current = halfEdge;
    do {
        faceHalfs.push_back(current);
        current = current->getNext();
    } while (current && current != halfEdge);

    bool looped = !faceHalfs.empty() && 
                  (faceHalfs.back()->getNext() == faceHalfs.front());

    if (!looped && !faceHalfs.empty()) {
        faceHalfs.pop_back();
    }

    // Calculate angles
    std::vector<float> angles;
    for (auto* half : faceHalfs) {
        angles.push_back(half->getAngle());
    }

    // These halfEdges turn clockwise around the face. Reverse for counter-clockwise.
    std::reverse(angles.begin(), angles.end());

    // Transform angles
    for (auto& angle : angles) {
        angle = Util::fixAngle(angle + M_PI);
    }

    if (!looped && !angles.empty()) {
        angles.insert(angles.begin(), Util::fixAngle(angles[0] + M_PI));
    } else if (!angles.empty()) {
        angles.push_back(angles[0]);
    }

    // Calculate turns
    turns = Util::wedgeTurns(angles);
}

std::string PrimalFace::boundaryString() const {
    std::string result;
    
    // Add turn indicators
    for (int i = 0; i < turns; ++i) {
        result += '^';
    }
    for (int i = 0; i > turns; --i) {
        result += 'v';
    }

    // Add boundary string from interior if available
    if (auto* halfEdge = getInterior() ? 
        static_cast<FaceNet*>(getInterior())->getOuterComponent() : nullptr) {
        result += halfEdge->boundaryString();
    }

    return result;
}

void PrimalFace::print() const {
    std::cout << boundaryString() << std::endl;
    if (getInterior()) {
        getInterior()->print();
    }
}

PrimalFace* PrimalFace::copy() const {
    return new PrimalFace(type, turns, wildcard);
}

void PrimalFace::import(BoundNet* boundNet, Types* types, const Json& json) {
    // Get interior connection
    int interiorIndex = json["interior"].get<int>();
    if (interiorIndex >= 0) {
        auto* interiorB = boundNet->getInterior()->getFaces()[interiorIndex];
        BoundNet::connectInterior(this, interiorB);
    }

    // Get boundary connection
    int boundaryIndex = json["boundary"].get<int>();
    if (boundaryIndex >= 0) {
        auto* boundaryB = boundNet->getBoundary()->getFaces()[boundaryIndex];
        BoundNet::connectBoundary(this, boundaryB);
    }

    // Set properties
    type = types->faceTypes[json["type"].get<int>()];
    turns = json["turns"].get<int>();
    wildcard = json["wildcard"].get<bool>();
}

Json PrimalFace::export() const {
    Json json;
    
    // Export connections
    auto* i = getInterior();
    auto* b = getBoundary();
    json["interior"] = i ? i->getNetwork()->faceIndex(i) : -1;
    json["boundary"] = b ? b->getNetwork()->faceIndex(b) : -1;
    
    // Export properties
    json["type"] = type->getNetwork()->getBoundNet()->getTypes()->faceTypes.indexOf(type);
    json["turns"] = turns;
    json["wildcard"] = wildcard;
    
    return json;
}

} // namespace ms 