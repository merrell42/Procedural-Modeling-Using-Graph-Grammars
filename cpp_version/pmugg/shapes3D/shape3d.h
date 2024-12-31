#pragma once
#include <vector>
#include <string>
#include <map>
#include "../guidelines/vertex_type.h"
#include "edge_type3d.h"
#include "face_type3d.h"

namespace ms {

class VertexType;
class EdgeType3D;

class Shape3D {
public:
    Shape3D();
    Shape3D(const std::vector<VertexType*>& vertexTypes,
            const std::vector<EdgeType3D*>& edgeTypes,
            const std::vector<FaceType3D*>& faceTypes,
            const std::string& xml);

    // Export/Import
    // std::map<std::string, std::vector<std::map<std::string, std::any>>> export_() const;
    static Shape3D* import(const Json& json);

    // Network operations
    Shape3D* getShape();
    std::vector<int> getVertices() const;
    /*EdgeType3D* getSpliceEdgeType(FaceType3D* faceType, int dir);

    static EdgeType3D* getSpliceEdgeType(
        std::map<std::string, EdgeType3D*>& splicedEdgeTypes,
        std::vector<EdgeType3D*>& edgeTypes,
        FaceType3D* faceType,
        int dir);*/
    std::vector<VertexType*> vertexTypes;
    std::vector<EdgeType3D*> edgeTypes;
    std::vector<FaceType3D*> faceTypes;

private:
    std::map<std::string, EdgeType3D*> splicedEdgeTypes;
    bool is3D;
    std::string xml;

    // Network data
    // ChainMap* chainMap;
    // VertexNetwork* vertex;
    // EdgeNetwork* edge;
    // FaceNetwork* face;
};

} // namespace ms 