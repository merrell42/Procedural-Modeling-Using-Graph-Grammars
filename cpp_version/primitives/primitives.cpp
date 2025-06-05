#include "pch.h"
#include "primitives.h"

namespace ms {

Primitives::Primitives(const std::vector<VertexType*>& vTypes,
                 const std::vector<EdgeType*>& eTypes,
                 const std::vector<FaceType*>& fTypes,
                 const std::string& xmlData,
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
    auto shape = new Primitives(dims);

    std::vector<FaceType*> faceTypes;
    for (const auto& type : json.at("faceTypes")) {
        faceTypes.push_back(FaceType::import(type));
    }
    shape->faceTypes = faceTypes;

    std::vector<EdgeType*> edgeTypes;
    for (const auto& type : json.at("edgeTypes")) {
        edgeTypes.push_back(EdgeType::import(type, shape));
    }
    shape->edgeTypes = edgeTypes;
    
    std::vector<VertexType*> vertexTypes;
    for (const auto& type : json.at("vertexTypes")) {
        vertexTypes.push_back(VertexType::import(type, shape));
    }
    shape->vertexTypes = vertexTypes;

    return shape;
}

Primitives* Primitives::getShape() {
    return this;
}

std::vector<int> Primitives::getVertices() const {
    return {0};
}

} // namespace ms 