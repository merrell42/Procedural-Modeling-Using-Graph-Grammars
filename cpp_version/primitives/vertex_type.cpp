#include "pch.h"
#include "vertex_type.h"
#include "edge_type.h"
#include "primitives.h"
#include "../util/util.h"
#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>

namespace ms {

int VertexType::nextId = 0;

VertexType::VertexType()
    : spliced(false)
    , id(nextId++) {
}

const vector<HalfEdgeType>& VertexType::getHalfEdgeTypes() const {
    return halfEdgeTypes;
}

bool VertexType::getSpliced() const {
    return spliced;
}

int VertexType::getId() const {
    return id;
}

void VertexType::addEdge(EdgeType* edge, bool isAtStart, double angle) {
    // Angle becomes the angle going out from the vertex through the edge.
    auto dir = edge->getDir();
    if (!isAtStart) {
        angle += (double)M_PI;
        dir = dir * -1.0f;
    }
    angle = ms::Util::fixAngle(angle);
    
    int directedId = 2 * edge->getId() + (isAtStart ? 0 : 1);
    double adjustedAngle = getAdjustedAngle(angle, edge, isAtStart);

    // Find insertion point to maintain sorted order.
    auto it = lower_bound(halfEdgeTypes.begin(), halfEdgeTypes.end(),
        HalfEdgeType{},
        [adjustedAngle, directedId](const HalfEdgeType& a, const HalfEdgeType& b) {
            return (a.adjustedAngle < adjustedAngle) ||
                   (a.adjustedAngle == adjustedAngle && a.directedId < directedId);
        });

    HalfEdgeType halfEdgeType;
    halfEdgeType.adjustedAngle = adjustedAngle;
    halfEdgeType.angle = angle;
    halfEdgeType.dir = dir;
    halfEdgeType.directedId = directedId;
    halfEdgeType.edge = edge;
    halfEdgeType.isAtStart = isAtStart;

    halfEdgeTypes.insert(it, halfEdgeType);
}

void VertexType::setSpliced(bool spliced) {
    this->spliced = spliced;
}

VertexType* VertexType::import(const Json& json, Primitives* shape) {    
    auto result = new VertexType();
    bool spliced = json["spliced"];
    result->spliced = spliced;

    for (const auto& halfEdgeTypeJson : json["connections"]) {
        HalfEdgeType halfEdgeType;
        halfEdgeType.edge = shape->edgeTypes[halfEdgeTypeJson["edge"].get<int>()];
        halfEdgeType.isAtStart = halfEdgeTypeJson["isAtStart"];
        if (!spliced) {
            halfEdgeType.adjustedAngle = halfEdgeTypeJson["adjustedAngle"];
            halfEdgeType.angle = halfEdgeTypeJson["angle"];
            halfEdgeType.dir = halfEdgeTypeJson.contains("dir") ? 
                Vec3::import(halfEdgeTypeJson["dir"]) : Vec3();
            halfEdgeType.directedId = halfEdgeTypeJson["directedId"];
        }
        result->halfEdgeTypes.push_back(halfEdgeType);
    }

    return result;
}

double VertexType::getAdjustedAngle(double angle, EdgeType* edge, bool isAtStart) {
    int directedId = 2 * edge->getId() + (isAtStart ? 0 : 1);
    return angle + 1e-5f * directedId;
}

} // namespace ms 