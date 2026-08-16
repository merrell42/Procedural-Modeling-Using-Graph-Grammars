#include "pch.h"
#include "vertex_type.h"
#include "edge_type.h"
#include "primitives.h"
#include "../util/util.h"
#include "../util/binary_stream.h"
#include <algorithm>

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
    if (json.contains("desirability")) {
        result->desirability = json["desirability"].get<double>();
    }

    for (const auto& halfEdgeTypeJson : json["halfEdgeTypes"]) {
        HalfEdgeType halfEdgeType;
        halfEdgeType.edge = shape->edgeTypes[halfEdgeTypeJson["edge"].get<int>()];
        halfEdgeType.isAtStart = halfEdgeTypeJson["isAtStart"];
        for (const auto& faceDatum : halfEdgeType.edge->getFaceData()) {
            halfEdgeType.faceIds.push_back(
                (int)indexOf(shape->faceTypes, faceDatum.type));
        }
        if (!spliced) {
            halfEdgeType.dir = halfEdgeTypeJson.contains("dir") ? 
                Vec3::import(halfEdgeTypeJson["dir"]) : Vec3();
        }
        result->halfEdgeTypes.push_back(halfEdgeType);
    }

    if (result->spliced) {
        result->computeFaceIdsForSpliced();
    }

    return result;
}

void VertexType::computeFaceIdsForSpliced() {
    const size_t N = halfEdgeTypes.size();
    for (size_t i = 0; i < N; i++) {
        vector<int> faceIds = { (int)i, (int)((i + 1) % N) };
        if (!halfEdgeTypes[i].isAtStart) {
            reverse(faceIds.begin(), faceIds.end());
        }
        halfEdgeTypes[i].faceIds = std::move(faceIds);
    }
}

HalfEdgeType::HalfEdgeType(EdgeType* newEdge, bool newIsAtStart, const vector<int>& newFaceIds)
    : dir()
    , edge(newEdge)
    , isAtStart(newIsAtStart)
    , faceIds(newFaceIds) {}

string HalfEdgeType::getId() const {
    return edge->getRuleGeneratorId() + (isAtStart ? "S" : "E");
}

int VertexType::getRuleGeneratorId() const {
    return ruleGeneratorId;
}

void VertexType::setRuleGeneratorId(int id) {
    ruleGeneratorId = id;
}

VertexType* VertexType::binaryDeserialize(std::istream& in, Primitives* shape) {
    auto* result = new VertexType();
    result->spliced         = bsRead<uint8_t>(in) != 0;
    result->ruleGeneratorId = bsRead<int32_t>(in);
    int32_t hetCount = bsRead<int32_t>(in);
    result->halfEdgeTypes.reserve(hetCount);
    for (int32_t i = 0; i < hetCount; i++) {
        int32_t etIdx  = bsRead<int32_t>(in);
        bool isAtStart = bsRead<uint8_t>(in) != 0;
        Vec3 dir       = bsReadVec3(in);
        HalfEdgeType het;
        het.edge      = shape->edgeTypes[etIdx];
        het.isAtStart = isAtStart;
        het.dir       = dir;
        result->halfEdgeTypes.push_back(het);
    }
    if (result->spliced) {
        result->computeFaceIdsForSpliced();
    }
    return result;
}

Json VertexType::exportJson(const Primitives* shape) const {
    Json json;
    json["spliced"] = spliced;
    if (desirability != 0.0) {
        json["desirability"] = desirability;
    }
    Json halfEdgeTypesJson = Json::array();
    for (const auto& halfEdgeType : halfEdgeTypes) {
        Json halfEdgeTypeJson;
        halfEdgeTypeJson["edge"] = indexOf(shape->edgeTypes, halfEdgeType.edge);
        halfEdgeTypeJson["isAtStart"] = halfEdgeType.isAtStart;
        if (!spliced) {
            halfEdgeTypeJson["dir"] = halfEdgeType.dir.exportJson();
        }
        halfEdgeTypesJson.push_back(std::move(halfEdgeTypeJson));
    }
    json["halfEdgeTypes"] = halfEdgeTypesJson;
    return json;
}
