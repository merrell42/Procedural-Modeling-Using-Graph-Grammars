#include "pch.h"
#include "primitives.h"

namespace ms {

Shape3D::Shape3D(const std::vector<VertexType*>& vTypes,
                 const std::vector<EdgeType3D*>& eTypes,
                 const std::vector<FaceType3D*>& fTypes,
                 const std::string& xmlData,
                 int dims)
    : vertexTypes(vTypes)
    , edgeTypes(eTypes)
    , faceTypes(fTypes)
    , xml(xmlData)
    , dims(dims) {}

Shape3D::Shape3D(int dims) : Shape3D({}, {}, {}, "", dims) {}

Shape3D* Shape3D::import(const Json& json) {
    int dims = 3;
    if (json.contains("dims")) {
        dims = json.at("dims");
    }
    auto shape = new Shape3D(dims);

    std::vector<FaceType3D*> faceTypes;
    for (const auto& type : json.at("faceTypes")) {
        faceTypes.push_back(FaceType3D::import(type));
    }
    shape->faceTypes = faceTypes;

    std::vector<EdgeType3D*> edgeTypes;
    for (const auto& type : json.at("edgeTypes")) {
        edgeTypes.push_back(EdgeType3D::import(type, shape));
    }
    shape->edgeTypes = edgeTypes;
    
    std::vector<VertexType*> vertexTypes;
    for (const auto& type : json.at("vertexTypes")) {
        vertexTypes.push_back(VertexType::import(type, shape));
    }
    shape->vertexTypes = vertexTypes;

    return shape;
}

Shape3D* Shape3D::getShape() {
    return this;
}

std::vector<int> Shape3D::getVertices() const {
    return {0};
}

} // namespace ms 