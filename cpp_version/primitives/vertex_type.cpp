#include "pch.h"
#include "vertex_type.h"
#include "edge_type.h"
#include "primitives.h"
#include "../util/util.h"

VertexType::VertexType() : spliced(false) {
}

const vector<HalfEdgeType>& VertexType::getHalfEdgeTypes() const {
    return halfEdgeTypes;
}

bool VertexType::getSpliced() const {
    return spliced;
}

void VertexType::setSpliced(bool spliced) {
    this->spliced = spliced;
}

void VertexType::addHalfEdge(EdgeType* edge, bool isAtStart) {
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

VertexType* VertexType::import(const Json& json, Primitives* shape) {    
    auto result = new VertexType();
    bool spliced = json["spliced"];
    result->spliced = spliced;

    for (const auto& halfEdgeTypeJson : json["halfEdgeTypes"]) {
        HalfEdgeType halfEdgeType;
        halfEdgeType.edge = shape->edgeTypes[halfEdgeTypeJson["edge"].get<int>()];
        halfEdgeType.isAtStart = halfEdgeTypeJson["isAtStart"];
        if (!spliced) {
            halfEdgeType.dir = halfEdgeTypeJson.contains("dir") ? 
                Vec3::import(halfEdgeTypeJson["dir"]) : Vec3();
        }
        result->halfEdgeTypes.push_back(halfEdgeType);
    }

    return result;
}

HalfEdgeType::HalfEdgeType(EdgeType* newEdge, bool newIsAtStart, const vector<int>& faceIds)
    : dir()
    , edge(newEdge)
    , isAtStart(newIsAtStart) {}
