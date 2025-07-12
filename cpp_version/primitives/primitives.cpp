#include "pch.h"
#include "primitives.h"

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
