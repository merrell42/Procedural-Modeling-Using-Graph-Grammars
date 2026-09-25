#pragma once
#include <vector>
#include <string>
#include <map>
#include "vertex_type.h"
#include "edge_type.h"
#include "face_type.h"
#include "../third_party/json.h"

class VertexType;
class EdgeType;
class Decorations;

using Json = nlohmann::json;
using namespace std;

// A collection of all the primitive types: vertex, edge, and face types.
class Primitives {
public:
    Primitives(int dims);
    Primitives(const vector<VertexType*>& vertexTypes,
            const vector<EdgeType*>& edgeTypes,
            const vector<FaceType*>& faceTypes,
            const string& xml,
            int dims);
    ~Primitives();
    static Primitives* import(const Json& json, const Json& decorationsJson = Json(), const string& assetDirectory = "");
    Json exportJson() const;

    vector<VertexType*> vertexTypes;
    vector<EdgeType*> edgeTypes;
    vector<FaceType*> faceTypes;
    int dims;

    Decorations* getDecorations() const;

private:
    map<string, EdgeType*> splicedEdgeTypes;
    string xml;
    Decorations* decorations;
};
