#include "net_transistor_settings.h"
#include "../util/util.h"

namespace ms {  

NetTransistorSettings::NetTransistorSettings(/* const MutationArea& mutationArea */)
    : lower(Vec3(1e10, 1e10, 1e10))
    , upper(Vec3(-1e10, -1e10, -1e10)) {}

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

}