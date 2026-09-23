#pragma once
#include <map>
#include <string>
#include "vertex_decoration.h"
#include "edge_decoration.h"
#include "face_decoration.h"
#include "../third_party/json.h"

using Json = nlohmann::json;
using namespace std;

class Decorations {
    public:
        Decorations() = default;
        ~Decorations();

        static Decorations* import(const Json& json);
        Json exportJson() const;

        VertexDecoration* getVertexDecoration(const string& id) const;
        EdgeDecoration* getEdgeDecoration(const string& id) const;
        FaceDecoration* getFaceDecoration(const string& id) const;

    private:
        Json sourceJson = Json{
            {"vertex", Json::array()},
            {"edge", Json::array()},
            {"face", Json::array()}
        };
        map<string, VertexDecoration*> vertexDecorations;
        map<string, EdgeDecoration*> edgeDecorations;
        map<string, FaceDecoration*> faceDecorations;
};
