#include "edge_placement.h"
#include "../graph_drawing/line.h"
#include "../graph_drawing/endpoint.h"
#include "../graph_drawing/vertex.h"
#include "../util/range.h"
#include "../decoration/brush.h"
#include "../fragment/net_transistor_settings.h"
#include "face_placement.h"

namespace ms {

EdgePlacement::EdgePlacement(Line* edge, int id, NetTransistorSettings* settings)
    : edge(edge)
    , id(id)
    , settings(settings) {
    
    // Get vertex IDs from endpoints
    auto endpoints = edge->getEndpoints();
    for (auto* endpoint : endpoints) {
        vertexIds.push_back(endpoint->getVertex()->getId());
    }

    // Handle self-loops
    if (vertexIds.size() == 1) {
        auto* endpoint = edge->getEndpoints()[0]->next();
        vertexIds.push_back(endpoint->getVertex()->getId());
    }

    // Set direction from first endpoint
    dir = edge->getEndpoints()[0]->getDir();
}

void EdgePlacement::initialize() {
    auto faces0 = settings->getVertex(vertexIds[0])->freeFaceIds;
    auto faces1 = settings->getVertex(vertexIds[1])->freeFaceIds;

    // Find intersection of free faces
    std::vector<int> intersection;
    for (int id : faces0) {
        if (std::find(faces1.begin(), faces1.end(), id) != faces1.end()) {
            intersection.push_back(id);
        }
    }

    // Get unique non-coplanar faces
    std::vector<int> faceIds = {intersection[0]};
    auto* fPlace0 = settings->getFace(intersection[0]);
    
    for (size_t i = 1; i < intersection.size(); i++) {
        auto* fPlaceI = settings->getFace(intersection[i]);
        if (!fPlace0->coplanar(fPlaceI)) {
            faceIds.push_back(intersection[i]);
        }
    }

    // Create new face if needed
    if (faceIds.size() < 2) {
        auto normal = fPlace0->getNormal().cross(dir);
        auto id = settings->createFace(normal);

        settings->getVertex(vertexIds[0])->addFreeFace(id);
        settings->getVertex(vertexIds[1])->addFreeFace(id);
    }
}

void EdgePlacement::addConstraint(int id) {
    if (std::find(constraints.begin(), constraints.end(), id) == constraints.end()) {
        constraints.push_back(id);
    }

    if (constraints.size() == 2) {
        settings->addToOrder(this->id, "edge", -1);
    }
}

Range EdgePlacement::getRange() const {
    auto* vPlace0 = settings->getVertex(vertexIds[0]);
    auto* vPlace1 = settings->getVertex(vertexIds[1]);
    
    auto mb0 = vPlace0->getChangeMB();
    auto mb1 = vPlace1->getChangeMB();
    
    auto mLength = dir.dot(mb1.m - mb0.m);
    auto bLength = dir.dot(mb1.b - mb0.b);

    auto* brush = edge ? edge->getEdgeType()->getBrush() : nullptr;
    float lengthMin = brush ? brush->getFloat("Min Length") : NetTransistorSettings::defaultLengthMin;
    float lengthMax = brush ? brush->getFloat("Max Length") : NetTransistorSettings::defaultLengthMax;
    float tileLength = 0;
    
    if (brush && brush->getBool("Rigid Tiled")) {
        tileLength = brush->getFloat("Tile Length");
    }

    return Range::transformCreate(mLength, bLength, Range(lengthMin, lengthMax, tileLength));
}

//void EdgePlacement::print() const {
//    if (edge) {
//        edge->print();
//    }
//}

} // namespace ms 