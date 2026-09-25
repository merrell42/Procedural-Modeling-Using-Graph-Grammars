#pragma once
#include "vertex_decoration.h"
#include "edge_decoration.h"
#include "face_decoration.h"
#include "../third_party/json.h"
#include <string>

using Json = nlohmann::json;

class DecorationsFactory {
    public:
        static void setAssetDirectory(const std::string& directory);
        static VertexDecoration* createVertexDecoration(const Json& json);
        static EdgeDecoration* createEdgeDecoration(const Json& json);
        static FaceDecoration* createFaceDecoration(const Json& json);
};
