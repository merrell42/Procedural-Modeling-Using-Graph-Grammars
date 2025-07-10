#include "pch.h"
#include "edge_placement.h"
#include "../graph_drawing/edge.h"
#include "../graph_drawing/half_edge.h"
#include "../graph_drawing/vertex.h"
#include "../util/range.h"
#include "../util/util.h"
#include "../graph/edge_settings.h"
#include "../grammar_rules/rule_applier_settings.h"
#include "face_placement.h"
#include "../memory_counter.h"

EdgePlacement::EdgePlacement(Edge* edge, int id, RuleApplierSettings* settings)
    : edge(edge)
    , id(id)
    , settings(settings) {
    MemoryCounter::creation("EdgePlacement");
    
    // Find the vertices at both ends of the edge.
    auto halfEdges = edge->getHalfEdges();
    for (auto* halfEdge : halfEdges) {
        if (halfEdge) {
            // Can be null in the ground rule.
            vertexIds.push_back(halfEdge->getVertex()->getId());
        }
    }

    // If the edge has only one half-edge, use the next vertex too.
    if (vertexIds.size() == 1) {
        auto* halfEdge = edge->getHalfEdges()[0]->next();
        vertexIds.push_back(halfEdge->getVertex()->getId());
    }

    // Set direction from first half-edge.
    dir = edge->getHalfEdges()[0]->getDir();
}

void EdgePlacement::initialize() {
    auto faces0 = settings->getVertex(vertexIds[0])->freeFaceIds;
    auto faces1 = settings->getVertex(vertexIds[1])->freeFaceIds;

    // Find all free faces that are in both vertices.
    vector<int> commonFaces;
    for (int id : faces0) {
        if (contains(faces1, id)) {
            commonFaces.push_back(id);
        }
    }

    // Find all non-coplanar faces.
    vector<int> faceIds = {commonFaces[0]};
    auto* fPlace0 = settings->getFace(commonFaces[0]);
    for (size_t i = 1; i < commonFaces.size(); i++) {
        auto* fPlaceI = settings->getFace(commonFaces[i]);
        if (!fPlace0->coplanar(fPlaceI)) {
            faceIds.push_back(commonFaces[i]);
        }
    }

    // Edges need two non-coplanar faces to set their position.
    // If one is missing, create a face orthogonal to the existing one.
    if (faceIds.size() < 2) {
        auto normal = fPlace0->getNormal().cross(dir);
        auto id = settings->createFace(normal);

        settings->getVertex(vertexIds[0])->addFreeFace(id);
        settings->getVertex(vertexIds[1])->addFreeFace(id);
    }
}

void EdgePlacement::addConstraint(int id) {
    if (!contains(constraints, id)) {
        constraints.push_back(id);
    }
    if (constraints.size() == 2) {
        settings->addToOrder(this->id, OrderInfo::Type::Edge, -1);
    }
}

// Find the range that leaves the edge with an acceptable length between min and max.
Range EdgePlacement::getRange() const {
    auto* vPlace0 = settings->getVertex(vertexIds[0]);
    auto* vPlace1 = settings->getVertex(vertexIds[1]);

    // mb defines a line that starts at b and goes in the m direction.
    auto mb0 = vPlace0->getChangeMB();
    auto mb1 = vPlace1->getChangeMB();

    // Calculate how m and b affect the edge length.
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
