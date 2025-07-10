#include "pch.h"
#include "rule_applier_settings.h"
#include "../util/util.h"

VertexPlacement* RuleApplierSettings::getVertex(int id) { return vertexPlacements[id].get(); }
EdgePlacement* RuleApplierSettings::getEdge(int id) { return edgePlacements[id].get(); }
FacePlacement* RuleApplierSettings::getFace(int id) { return facePlacements[id].get(); }

void RuleApplierSettings::addToOrder(int id, OrderInfo::Type type, int vertexId) {
    if (find(orderIds.begin(), orderIds.end(), id) == orderIds.end()) {
        orderIds.push_back(id);
        orderInfo.push_back({type, vertexId});
    }
}

int RuleApplierSettings::createFace(const Vec3& normal) {
    int id = newFaceCounter--;
    facePlacements[id] = make_unique<FacePlacement>(normal, id, this, nullptr);
    return id;
}

// Merges two coplanar faces so they are treated as one.
// All references to idB are replaced with idA.
void RuleApplierSettings::mergeFace(int idA, int idB) {
    uniqueFaceMap[idB] = idA;
    
    // Update all face mappings.
    for (auto& [key, value] : uniqueFaceMap) {
        if (value == idB) {
            value = idA;
        }
    }

    // Update vertex placements.
    for (auto& [key, vPlace] : vertexPlacements) {
        for (auto& faceId : vPlace->freeFaceIds) {
            if (faceId == idB) {
                faceId = idA;
                int vId = key;
                Util::remove<int>(getFace(idB)->vertexIds, vId);
                getFace(idA)->vertexIds.push_back(vId);
            }
        }
        for (auto& faceId : vPlace->constrainedFaceIds) {
            if (faceId == idB) {
                faceId = idA;
                int vId = key;
                Util::remove<int>(getFace(idB)->vertexIds, vId);
                getFace(idA)->vertexIds.push_back(vId);
            }
        }
    }
}

// Finds the index where the basis face in the orderIds.
int RuleApplierSettings::findBasisIndex(const int basisId) {
    for (size_t index = 0; index < orderIds.size(); ++index) {
        if (orderIds[index] == basisId && orderInfo[index].type == OrderInfo::Type::Face) {
            return static_cast<int>(index);
        }
    }
    return -1;
}
