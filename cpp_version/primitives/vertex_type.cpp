#include "pch.h"
#include "vertex_type.h"
#include "edge_type.h"
#include "primitives.h"
#include "../util/util.h"
#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>

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

void VertexType::addEdge(EdgeType* edge, bool isAtStart) {
    auto dir = edge->getDir();
    if (!isAtStart) {
        dir = dir * -1.0f;
    }

    HalfEdgeType halfEdgeType;
    halfEdgeType.dir = dir;
    halfEdgeType.edge = edge;
    halfEdgeType.isAtStart = isAtStart;

    halfEdgeTypes.push_back(halfEdgeType);
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
            // TODO: Remove unneeded JSON values.
            // halfEdgeType.adjustedAngle = halfEdgeTypeJson["adjustedAngle"];
            // halfEdgeType.angle = halfEdgeTypeJson["angle"];
            halfEdgeType.dir = halfEdgeTypeJson.contains("dir") ? 
                Vec3::import(halfEdgeTypeJson["dir"]) : Vec3();
            // halfEdgeType.directedId = halfEdgeTypeJson["directedId"];
        }
        result->halfEdgeTypes.push_back(halfEdgeType);
    }

    return result;
}

HalfEdgeType::HalfEdgeType(EdgeType* newEdge, bool newIsAtStart, const vector<int>& faceIds)
    : dir()
    , edge(newEdge)
    , isAtStart(newIsAtStart) {}
