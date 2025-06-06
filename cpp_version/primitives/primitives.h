#pragma once
#include <vector>
#include <string>
#include <map>
#include "vertex_type.h"
#include "edge_type.h"
#include "face_type.h"

class VertexType;
class EdgeType;

class Primitives {
public:
    Primitives(int dims);
    Primitives(const vector<VertexType*>& vertexTypes,
            const vector<EdgeType*>& edgeTypes,
            const vector<FaceType*>& faceTypes,
            const string& xml,
            int dims);
    static Primitives* import(const Json& json);

    Primitives* getShape();
    vector<int> getVertices() const;

    vector<VertexType*> vertexTypes;
    vector<EdgeType*> edgeTypes;
    vector<FaceType*> faceTypes;
    int dims;

private:
    map<string, EdgeType*> splicedEdgeTypes;
    string xml;
};

