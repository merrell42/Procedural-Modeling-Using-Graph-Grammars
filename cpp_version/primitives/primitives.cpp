#include "pch.h"
#include "primitives.h"
#include "../util/util.h"

#include <string>

Primitives::Primitives(const vector<VertexType*>& vTypes,
                 const vector<EdgeType*>& eTypes,
                 const vector<FaceType*>& fTypes,
                 const string& xmlData,
                 int dims)
    : vertexTypes(vTypes)
    , edgeTypes(eTypes)
    , faceTypes(fTypes)
    , xml(xmlData)
    , dims(dims) {}

Primitives::Primitives(int dims) : Primitives({}, {}, {}, "", dims) {}

Primitives* Primitives::import(const Json& json) {
    int dims = 3;
    if (json.contains("dims")) {
        dims = json.at("dims");
    }
    auto primitives = new Primitives(dims);

    vector<FaceType*> faceTypes;
    for (const auto& type : json.at("faceTypes")) {
        faceTypes.push_back(FaceType::import(type));
    }
    primitives->faceTypes = faceTypes;

    vector<EdgeType*> edgeTypes;
    for (const auto& type : json.at("edgeTypes")) {
        edgeTypes.push_back(EdgeType::import(type, primitives));
    }
    primitives->edgeTypes = edgeTypes;
    
    vector<VertexType*> vertexTypes;
    for (const auto& type : json.at("vertexTypes")) {
        vertexTypes.push_back(VertexType::import(type, primitives));
    }
    primitives->vertexTypes = vertexTypes;

    return primitives;
}

Json Primitives::exportJson() const {
    Json json;
    json["dims"] = dims;
    Json faceTypesJson = Json::array();
    for (const auto* faceType : faceTypes) {
        faceTypesJson.push_back(faceType->exportJson());
    }
    json["faceTypes"] = faceTypesJson;
    Json edgeTypesJson = Json::array();
    for (const auto* edgeType : edgeTypes) {
        edgeTypesJson.push_back(edgeType->exportJson(this));
    }
    json["edgeTypes"] = edgeTypesJson;
    Json vertexTypesJson = Json::array();
    for (const auto* vertexType : vertexTypes) {
        vertexTypesJson.push_back(vertexType->exportJson(this));
    }
    json["vertexTypes"] = vertexTypesJson;
    return json;
}

EdgeType* Primitives::getSplicedEdgeType(FaceType* faceType) {
    int faceIndex = indexOf(faceTypes, faceType);
    if (faceIndex < 0) {
        throw runtime_error("getSplicedEdgeType: unknown face type");
    }
    string key = to_string(faceIndex);
    auto it = splicedEdgeTypes.find(key);
    if (it != splicedEdgeTypes.end()) {
        return it->second;
    }

    vector<FaceData> faceData = {
        { faceType, true },
        { faceType, false },
    };
    auto* result = new EdgeType(faceData, Vec3(0, 1, 0), false);
    result->setSpliced(true);
    splicedEdgeTypes[key] = result;
    return result;
}
