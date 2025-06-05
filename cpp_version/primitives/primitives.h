#pragma once
#include <vector>
#include <string>
#include <map>
#include "vertex_type.h"
#include "edge_type.h"
#include "face_type.h"

namespace ms {

class VertexType;
class EdgeType;

class Shape3D {
public:
    Shape3D(int dims);
    Shape3D(const std::vector<VertexType*>& vertexTypes,
            const std::vector<EdgeType*>& edgeTypes,
            const std::vector<FaceType*>& faceTypes,
            const std::string& xml,
            int dims);
    static Shape3D* import(const Json& json);

    Shape3D* getShape();
    std::vector<int> getVertices() const;

    std::vector<VertexType*> vertexTypes;
    std::vector<EdgeType*> edgeTypes;
    std::vector<FaceType*> faceTypes;
    int dims;

private:
    std::map<std::string, EdgeType*> splicedEdgeTypes;
    std::string xml;
};

} // namespace ms 