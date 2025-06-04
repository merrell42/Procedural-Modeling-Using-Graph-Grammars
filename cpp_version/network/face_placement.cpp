#include "pch.h"
#include "face_placement.h"
#include "../graph_drawing/face.h"
#include "../fragment/net_transistor_settings.h"
#include "vertex_placement.h"
#include "../util/util.h"
#include "../util/range.h"
#include <iostream>

namespace ms {

FacePlacement::FacePlacement(const Vec3& normal, int id, NetTransistorSettings* settings, Face* face)
    : normal(normal)
    , face(face)
    , id(id)
    , settings(settings) {}

Face* FacePlacement::getFace() const {
    return face;
}

double FacePlacement::getD() const {
    return d;
}

bool FacePlacement::isFree() const {
    return free;
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
    if (std::find(vertexIds.begin(), vertexIds.end(), id) == vertexIds.end()) {
        vertexIds.push_back(id);
    }
}

void FacePlacement::addFixedNeighbor(Face* neighbor) {
    Util::union_(fixedNeighbors, {neighbor});
}

bool FacePlacement::coplanar(FacePlacement* fPlaceB) const {
    const Vec3& nA = getNormal();
    const Vec3& nB = fPlaceB->getNormal();
    return std::abs(nA.dot(nB)) > 1.0 - 1e-4;
}

void FacePlacement::constrain(bool addBasis, int vertexId) {
    settings->addToOrder(id, "face", vertexId);

    if (addBasis) {
        settings->basisIds.push_back(id);
    }
    free = false;

    for (int id : vertexIds) {
        auto* vPlace = settings->getVertex(id);
        vPlace->constrainFace(this->id);
    }
    for (int id : vertexIds) {
        settings->getVertex(id)->propagate();
    }
}

void FacePlacement::setD(double d) {
    this->d = d;
    this->slope = 0;
    this->value = d;
}

bool FacePlacement::setFromVertex(int vertexId) {
    auto* vPlace = settings->getVertex(vertexId);
    Vec3 pos = vPlace->getPosition();
    double d = normal.dot(pos);
    
    if (fixed && std::abs(d - this->d) > 1e-4) {
        return false;
    }
    
    setD(d);
    return true;
}

void FacePlacement::makeFixed(const FixedFace& fixedFace) {
    for (int id : vertexIds) {
        settings->getVertex(id)->addFixedNeighbor(fixedFace);
    }
    constrain(true, -1);
    fixedFace.faceA->getGroup()->connectHole(face->getGroup());
}

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
        if (m == 0) {
            return Range(-std::numeric_limits<double>::infinity(),
                        std::numeric_limits<double>::infinity());
        }
        return Range((double)value, (double)value);
    }

    slope = m;
    value = b;

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

    Range range((double)lowD, (double)highD);
    for (auto* neighbor : fixedNeighbors) {
        Range rangeI = neighbor->dirBounds(normal);
        range = range.intersect(rangeI);
    }

    return Range::transformCreate((double)m, (double)b, range);
}

ChangeMB FacePlacement::getChangeMB() const {
    double m = slope;
    double b = value;
    return {m, b};
}

} // namespace ms 