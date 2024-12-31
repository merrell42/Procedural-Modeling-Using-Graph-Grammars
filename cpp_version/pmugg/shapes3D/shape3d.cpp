#include "shape3d.h"

namespace ms {

Shape3D::Shape3D(const std::vector<VertexType*>& vTypes,
                 const std::vector<EdgeType3D*>& eTypes,
                 const std::vector<FaceType3D*>& fTypes,
                 const std::string& xmlData)
    : vertexTypes(vTypes)
    , edgeTypes(eTypes)
    , faceTypes(fTypes)
    , is3D(true)
    , xml(xmlData)
    // , chainMap(nullptr)
    // , vertex(nullptr)
    // , edge(nullptr)
    // , face(nullptr)
        {}

Shape3D::Shape3D() : Shape3D({}, {}, {}, "") {}

//std::map<std::string, std::vector<std::map<std::string, std::any>>> Shape3D::export_() const {
//    auto exporter = [this](const auto& x) { return x.export_(this); };
//    
//    return {
//        {"vertexTypes", std::vector<std::map<std::string, std::any>>(
//            vertexTypes.begin(), vertexTypes.end(), exporter)},
//        {"edgeTypes", std::vector<std::map<std::string, std::any>>(
//            edgeTypes.begin(), edgeTypes.end(), exporter)},
//        {"faceTypes", std::vector<std::map<std::string, std::any>>(
//            faceTypes.begin(), faceTypes.end(), exporter)},
//        {"xml", xml}
//    };
//}

Shape3D* Shape3D::import(const Json& json) {
    auto shape = new Shape3D();

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

//EdgeType3D* Shape3D::getSpliceEdgeType(
//    std::map<std::string, EdgeType3D*>& splicedEdgeTypes,
//    std::vector<EdgeType3D*>& edgeTypes,
//    FaceType3D* faceType,
//    int dir) {
//    
//    std::string key = std::to_string(faceType->id) + "," + std::to_string(dir);
//    
//    if (!splicedEdgeTypes.count(key)) {
//        std::vector<FaceData> faceData = {
//            {faceType, true},
//            {faceType, false}
//        };
//        
//        auto uv = (dir == 0) ? faceType->u : faceType->v;
//        auto edgeType = new EdgeType3D(faceData, uv);
//        edgeType->setSpliced(true);
//        
//        splicedEdgeTypes[key] = edgeType;
//        edgeTypes.push_back(edgeType);
//    }
//    
//    return splicedEdgeTypes[key];
//}
//
//EdgeType3D* Shape3D::getSpliceEdgeType(FaceType3D* faceType, int dir) {
//    return getSpliceEdgeType(splicedEdgeTypes, edgeTypes, faceType, dir);
//}

} // namespace ms 