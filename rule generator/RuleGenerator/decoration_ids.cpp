// Rule generation keeps decoration ids so exported grammars still name them.
// The drawing implementations live in cpp_version and are not linked here.

#include "pch.h"
#include "../../cpp_version/decorations/decorations.h"

namespace {

class IdVertexDecoration : public VertexDecoration {
public:
    DecorationOutput getOutput(const Matrix4&) const override { return {}; }
};

class IdEdgeDecoration : public EdgeDecoration {
public:
    DecorationOutput getOutput(const Vec3&, const Vec3&) const override { return {}; }
};

class IdFaceDecoration : public FaceDecoration {
public:
    DecorationOutput getOutput(const Face&) const override { return {}; }
};

template <typename Base, typename Concrete>
void importIds(const Json& items, map<string, Base*>& decorations) {
    for (const auto& item : items) {
        auto* decoration = new Concrete();
        decoration->setId(item.at("id").get<string>());
        decorations[decoration->getId()] = decoration;
    }
}

template <typename Base>
void deleteDecorations(map<string, Base*>& decorations) {
    for (auto& entry : decorations) {
        delete entry.second;
    }
    decorations.clear();
}

Json emptyDecorationJson() {
    return Json{
        {"vertex", Json::array()},
        {"edge", Json::array()},
        {"face", Json::array()}
    };
}

}  // namespace

Decorations::~Decorations() {
    deleteDecorations(vertexDecorations);
    deleteDecorations(edgeDecorations);
    deleteDecorations(faceDecorations);
}

Decorations* Decorations::import(const Json& json, const string&) {
    auto* result = new Decorations();
    if (json.is_null() || json.empty()) {
        result->sourceJson = emptyDecorationJson();
        return result;
    }
    result->sourceJson = json;
    if (json.contains("vertex")) {
        importIds<VertexDecoration, IdVertexDecoration>(json["vertex"], result->vertexDecorations);
    }
    if (json.contains("edge")) {
        importIds<EdgeDecoration, IdEdgeDecoration>(json["edge"], result->edgeDecorations);
    }
    if (json.contains("face")) {
        importIds<FaceDecoration, IdFaceDecoration>(json["face"], result->faceDecorations);
    }
    return result;
}

Json Decorations::exportJson() const {
    return sourceJson;
}

VertexDecoration* Decorations::getVertexDecoration(const string& id) const {
    auto it = vertexDecorations.find(id);
    return it == vertexDecorations.end() ? nullptr : it->second;
}

EdgeDecoration* Decorations::getEdgeDecoration(const string& id) const {
    auto it = edgeDecorations.find(id);
    return it == edgeDecorations.end() ? nullptr : it->second;
}

FaceDecoration* Decorations::getFaceDecoration(const string& id) const {
    auto it = faceDecorations.find(id);
    return it == faceDecorations.end() ? nullptr : it->second;
}
