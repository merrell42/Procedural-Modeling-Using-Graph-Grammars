#include "pch.h"
#include "vertex_type.h"
#include "edge_type.h"
#include "primitives.h"
#include "../util/util.h"

VertexType::VertexType() : spliced(false), ruleGeneratorId(0) {
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

string VertexConnection::getId() const {
    return edge->getRuleGeneratorId() + (isAtStart ? "S" : "E");
}

const vector<VertexConnection>& VertexType::getConnections() const {
    return connections;
}

int VertexType::getRuleGeneratorId() const {
    return ruleGeneratorId;
}

void VertexType::setRuleGeneratorId(int id) {
    ruleGeneratorId = id;
}

VertexType* VertexType::importRuleGenerator(const Json& json, const map<string, EdgeType*>& eTypes) {
    auto result = new VertexType();
    result->spliced = json.value("spliced", false);
    result->ruleGeneratorId = json.value("id", 0);
    
    for (const auto& connJson : json["connections"]) {
        string edgeId = connJson["edge"].get<string>();
        bool isAtStart = connJson["isAtStart"];
        
        auto it = eTypes.find(edgeId);
        if (it != eTypes.end()) {
            result->connections.push_back(VertexConnection(it->second, isAtStart));
        }
    }
    
    return result;
}
