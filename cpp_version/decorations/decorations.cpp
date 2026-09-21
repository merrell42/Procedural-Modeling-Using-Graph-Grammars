#include "pch.h"
#include "decorations.h"
#include "place_object_decoration.h"
#include <stdexcept>

using namespace std;

namespace {

VertexDecoration* createVertexDecoration(const Json& json) {
    const string type = json.at("type").get<string>();
    if (type == "place object") {
        return new PlaceObjectDecoration(json.at("object").get<string>());
    }
    throw runtime_error("Unknown vertex decoration type: " + type);
}

EdgeDecoration* createEdgeDecoration(const Json& json) {
    const string type = json.at("type").get<string>();
    throw runtime_error("Unknown edge decoration type: " + type);
}

FaceDecoration* createFaceDecoration(const Json& json) {
    const string type = json.at("type").get<string>();
    throw runtime_error("Unknown face decoration type: " + type);
}

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

template <typename T>
string findId(const map<string, T*>& decorations, const T* decoration) {
    if (!decoration) {
        return "";
    }
    for (const auto& [id, value] : decorations) {
        if (value == decoration) {
            return id;
        }
    }
    return "";
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
        for (const auto& item : json["vertex"]) {
            const string id = item.at("id").get<string>();
            result->vertexDecorations[id] = createVertexDecoration(item);
        }
    }
    if (json.contains("edge")) {
        for (const auto& item : json["edge"]) {
            const string id = item.at("id").get<string>();
            result->edgeDecorations[id] = createEdgeDecoration(item);
        }
    }
    if (json.contains("face")) {
        for (const auto& item : json["face"]) {
            const string id = item.at("id").get<string>();
            result->faceDecorations[id] = createFaceDecoration(item);
        }
    }
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

string Decorations::getId(const VertexDecoration* decoration) const {
    return findId(vertexDecorations, decoration);
}

string Decorations::getId(const EdgeDecoration* decoration) const {
    return findId(edgeDecorations, decoration);
}

string Decorations::getId(const FaceDecoration* decoration) const {
    return findId(faceDecorations, decoration);
}
