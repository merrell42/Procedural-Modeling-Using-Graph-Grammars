#include "pch.h"
#include "decorations.h"
#include "decorations_factory.h"

using namespace std;

namespace {

template <typename T>
void deleteDecorations(map<string, T*>& decorations) {
    for (auto& [id, decoration] : decorations) {
        delete decoration;
    }
    decorations.clear();
}

template <typename T>
T* findDecoration(const map<string, T*>& decorations, const string& id) {
    auto it = decorations.find(id);
    if (it == decorations.end()) {
        return nullptr;
    }
    return it->second;
}

template <typename Base>
void resolveAll(const map<string, Base*>& decorations) {
    for (auto& entry : decorations) {
        if (entry.second) {
            entry.second->resolveChildren(decorations);
        }
    }
}

template <typename Base>
void importKind(const Json& items, map<string, Base*>& decorations, Base* (*create)(const Json&)) {
    for (const auto& item : items) {
        Base* decoration = create(item);
        decoration->setId(item.at("id").get<string>());
        decorations[decoration->getId()] = decoration;
    }
}

} // namespace

Decorations::~Decorations() {
    deleteDecorations(vertexDecorations);
    deleteDecorations(edgeDecorations);
    deleteDecorations(faceDecorations);
}

Decorations* Decorations::import(const Json& json) {
    auto* result = new Decorations();
    if (json.is_null() || json.empty()) {
        result->sourceJson = Json{
            {"vertex", Json::array()},
            {"edge", Json::array()},
            {"face", Json::array()}
        };
        return result;
    }
    result->sourceJson = json;

    if (json.contains("vertex")) {
        importKind(json["vertex"], result->vertexDecorations, DecorationsFactory::createVertexDecoration);
    }
    if (json.contains("edge")) {
        importKind(json["edge"], result->edgeDecorations, DecorationsFactory::createEdgeDecoration);
    }
    if (json.contains("face")) {
        importKind(json["face"], result->faceDecorations, DecorationsFactory::createFaceDecoration);
    }

    resolveAll(result->vertexDecorations);
    resolveAll(result->edgeDecorations);
    resolveAll(result->faceDecorations);
    return result;
}

Json Decorations::exportJson() const {
    return sourceJson;
}

VertexDecoration* Decorations::getVertexDecoration(const string& id) const {
    return findDecoration(vertexDecorations, id);
}

EdgeDecoration* Decorations::getEdgeDecoration(const string& id) const {
    return findDecoration(edgeDecorations, id);
}

FaceDecoration* Decorations::getFaceDecoration(const string& id) const {
    return findDecoration(faceDecorations, id);
}
