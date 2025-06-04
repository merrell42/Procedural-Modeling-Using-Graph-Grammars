#include "pch.h"
#include "rule_applier_settings.h"
#include "../util/util.h"

namespace ms {  

void NetTransistorSettings::setVertex(int id, std::unique_ptr<VertexPlacement> vPlace) {
    vertexPlacements[id] = std::move(vPlace);
}

void NetTransistorSettings::setEdge(int id, std::unique_ptr<EdgePlacement> ePlace) {
    edgePlacements[id] = std::move(ePlace);
}

void NetTransistorSettings::setFace(int id, std::unique_ptr<FacePlacement> fPlace) {
    facePlacements[id] = std::move(fPlace);
}

void NetTransistorSettings::addToOrder(int id, const std::string& type, int vertexId) {
    if (std::find(orderIds.begin(), orderIds.end(), id) == orderIds.end()) {
        orderIds.push_back(id);
        orderInfo.push_back({type, vertexId});
    }
}

int NetTransistorSettings::createFace(const Vec3& normal) {
    int id = newFaceCounter--;
    facePlacements[id] = std::make_unique<FacePlacement>(normal, id, this, nullptr /* unsure about this*/);
    return id;
}

void NetTransistorSettings::mergeFace(int idA, int idB) {
    uniqueFaceMap[idB] = idA;
    
    // Update all face mappings
    for (auto& [key, value] : uniqueFaceMap) {
        if (value == idB) {
            value = idA;
        }
    }

    // Update vertex placements
    for (auto& [key, vPlace] : vertexPlacements) {
        for (auto& faceId : vPlace->freeFaceIds) {
            if (faceId == idB) {
                faceId = idA;
                int vId = key;
                Util::remove<int>(getFace(idB)->vertexIds, vId);
                getFace(idA)->vertexIds.push_back(vId);
            }
        }
        
        for (auto& faceId : vPlace->unfreeFaceIds) {
            if (faceId == idB) {
                faceId = idA;
                int vId = key;
                Util::remove<int>(getFace(idB)->vertexIds, vId);
                getFace(idA)->vertexIds.push_back(vId);
            }
        }
    }
}

int NetTransistorSettings::findBasisOrder(const int basisId) {
    for (size_t index = 0; index < orderIds.size(); ++index) {
        if (orderIds[index] == basisId && orderInfo[index].type == "face") {
            return static_cast<int>(index);
        }
    }
    return -1;
}

}