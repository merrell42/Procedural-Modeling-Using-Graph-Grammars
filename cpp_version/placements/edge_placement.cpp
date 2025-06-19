#include "pch.h"
#include "edge_placement.h"
#include "../graph_drawing/edge.h"
#include "../graph_drawing/half_edge.h"
#include "../graph_drawing/vertex.h"
#include "../util/range.h"
#include "../graph/edge_settings.h"
#include "../grammar_rules/rule_applier_settings.h"
#include "face_placement.h"
#include "../memory_counter.h"

EdgePlacement::EdgePlacement(Edge* edge, int id, RuleApplierSettings* settings)
    : edge(edge)
    , id(id)
    , settings(settings) {
    MemoryCounter::creation("EdgePlacement");
    
    // Get vertex IDs from halfEdges
    auto halfEdges = edge->getHalfEdges();
    for (auto* halfEdge : halfEdges) {
        if (halfEdge) {
            // Can be null in the ground rule.
            vertexIds.push_back(halfEdge->getVertex()->getId());
        }
    }

    // Handle self-loops
    if (vertexIds.size() == 1) {
        auto* halfEdge = edge->getHalfEdges()[0]->next();
        vertexIds.push_back(halfEdge->getVertex()->getId());
    }

    // Set direction from first halfEdge
    dir = edge->getHalfEdges()[0]->getDir();
}

void EdgePlacement::initialize() {
    auto faces0 = settings->getVertex(vertexIds[0])->freeFaceIds;
    auto faces1 = settings->getVertex(vertexIds[1])->freeFaceIds;

    // Find intersection of free faces
    vector<int> intersection;
    for (int id : faces0) {
        if (find(faces1.begin(), faces1.end(), id) != faces1.end()) {
            intersection.push_back(id);
        }
    }

    // Get unique non-coplanar faces
    vector<int> faceIds = {intersection[0]};
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
    if (find(constraints.begin(), constraints.end(), id) == constraints.end()) {
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

    auto* edgeSettings = edge ? edge->getEdgeType()->getEdgeSettings() : nullptr;
    double lengthMin = edgeSettings ? edgeSettings->getDouble("Min Length") : RuleApplierSettings::defaultLengthMin;
    double lengthMax = edgeSettings ? edgeSettings->getDouble("Max Length") : RuleApplierSettings::defaultLengthMax;
    double tileLength = 0;
    
    if (edgeSettings && edgeSettings->getBool("Rigid Tiled")) {
        tileLength = edgeSettings->getDouble("Tile Length");
    }

    return Range::transformCreate(mLength, bLength, Range(lengthMin, lengthMax, tileLength));
}
