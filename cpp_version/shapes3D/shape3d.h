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
    Shape3D(int dims);
    Shape3D(const std::vector<VertexType*>& vertexTypes,
            const std::vector<EdgeType3D*>& edgeTypes,
            const std::vector<FaceType3D*>& faceTypes,
            const std::string& xml,
            int dims);
    static Shape3D* import(const Json& json);

    Shape3D* getShape();
    std::vector<int> getVertices() const;

    std::vector<VertexType*> vertexTypes;
    std::vector<EdgeType3D*> edgeTypes;
    std::vector<FaceType3D*> faceTypes;
    int dims;

private:
    std::map<std::string, EdgeType3D*> splicedEdgeTypes;
    std::string xml;
};

} // namespace ms 