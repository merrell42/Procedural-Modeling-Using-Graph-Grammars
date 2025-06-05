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

class Primitives {
public:
    Primitives(int dims);
    Primitives(const std::vector<VertexType*>& vertexTypes,
            const std::vector<EdgeType*>& edgeTypes,
            const std::vector<FaceType*>& faceTypes,
            const std::string& xml,
            int dims);
    static Primitives* import(const Json& json);

    Primitives* getShape();
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