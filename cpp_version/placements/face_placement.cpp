#include "pch.h"
#include "face_placement.h"
#include "../graph_drawing/face.h"
#include "../grammar_rules/rule_applier_settings.h"
#include "vertex_placement.h"
#include "../util/util.h"
#include "../util/range.h"
#include <iostream>

FacePlacement::FacePlacement(const Vec3& normal, int id, RuleApplierSettings* settings, Face* face)
    : normal(normal)
    , face(face)
    , id(id)
    , settings(settings) {
    MemoryCounter::creation("FacePlacement");
}

FacePlacement::~FacePlacement() {
    MemoryCounter::destruction("FacePlacement");
}

Face* FacePlacement::getFace() const {
    return face;
}

double FacePlacement::getD() const {
    return d;
}

bool FacePlacement::isConstrained() const {
    return constrained;
}

const Vec3& FacePlacement::getNormal() const {
    return normal;
}

void FacePlacement::setFixed(bool fixed) {
    this->fixed = fixed;
}

bool FacePlacement::getFixed() const {
    return fixed;
}

void FacePlacement::addVertexId(int id) {
    if (!contains(vertexIds, id)) {
        vertexIds.push_back(id);
    }
}

void FacePlacement::addFixedNeighbor(Face* neighbor) {
    fixedNeighbors.insert(neighbor);
}

// Returns true if the two faces are coplanar.
bool FacePlacement::coplanar(FacePlacement* fPlaceB) const {
    const Vec3& nA = getNormal();
    const Vec3& nB = fPlaceB->getNormal();
    return abs(nA.dot(nB)) > 1.0 - 1e-4;
}

void FacePlacement::constrain(bool addBasis, int vertexId) {
    settings->addToOrder(id, OrderInfo::Type::Face, vertexId);

    if (addBasis) {
        settings->basisIds.push_back(id);
    }
    constrained = true;

    for (int id : vertexIds) {
        auto* vPlace = settings->getVertex(id);
        vPlace->constrainFace(this->id);
    }
    for (int id : vertexIds) {
        settings->getVertex(id)->propagate();
    }
}

void FacePlacement::setD(double newD) {
    d = newD;
    slope = 0;
    value = newD;
}

// Set the face position so it intersects the vertex.
bool FacePlacement::setFromVertex(int vertexId) {
    auto* vPlace = settings->getVertex(vertexId);
    Vec3 pos = vPlace->getPosition();
    double newD = normal.dot(pos);

    // If the face is already fixed, check that the new position is the same.
    if (fixed && abs(d - newD) > 1e-4) {
        return false;
    }
    
    setD(d);
    return true;
}

// Make the face fixed.
void FacePlacement::makeFixed(const FixedFace& fixedFace) {
    for (int id : vertexIds) {
        settings->getVertex(id)->addFixedNeighbor(fixedFace);
    }
    constrain(true, -1);
    fixedFace.faceA->getGroup()->merge(face->getGroup());
}

// Find the range of possible values for a vertex.
Range FacePlacement::getRange(int vertexId) {
    const auto& lower = settings->lower;
    const auto& upper = settings->upper;

    double m = 1.0;
    double b = 0.0;

    if (vertexId != -1) {
        auto* vPlace = settings->getVertex(vertexId);
        m = normal.dot(vPlace->slope);
        b = normal.dot(vPlace->value);
    }

    if (fixed) {
        // If the slope is 0, then any value is acceptable.
        if (m == 0) {
            return Range(-numeric_limits<double>::infinity(),
                        numeric_limits<double>::infinity());
        }
        // Otherwise, the face is fixed to a specific value.
        return Range((double)value, (double)value);
    }

    slope = m;
    value = b;

    // Require the face to be within the extents of the model.
    double lowD = 0;
    double highD = 0;
    for (int i = 0; i < 3; i++) {
        double ni = normal[i];
        if (ni > 0) {
            lowD += ni * lower[i];
            highD += ni * upper[i];
        } else {
            lowD += ni * upper[i];
            highD += ni * lower[i];
        }
    }
    // Do not allow the face to cut off other neighbor vertices.
    // They should not be removed by sliding the face.
    Range range((double)lowD, (double)highD);
    for (auto* neighbor : fixedNeighbors) {
        Range rangeI = neighbor->dirBounds(normal);
        range.intersect(rangeI);
    }

    return Range::transformCreate((double)m, (double)b, range);
}

// Returns the slope and value.
ChangeMB FacePlacement::getChangeMB() const {
    double m = slope;
    double b = value;
    return {m, b};
}
