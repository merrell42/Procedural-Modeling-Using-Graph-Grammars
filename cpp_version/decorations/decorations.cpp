#include "pch.h"
#include "decorations.h"
#include "place_object_decoration.h"
#include "union_decoration.h"
#include <iostream>
#include <stdexcept>

using namespace std;

namespace {

struct PendingUnion {
    string id;
    vector<string> childIds;
};

vector<string> readChildIds(const Json& json) {
    vector<string> childIds;
    if (!json.contains("childIds") || !json["childIds"].is_array()) {
        return childIds;
    }
    for (const auto& childId : json["childIds"]) {
        childIds.push_back(childId.get<string>());
    }
    return childIds;
}

VertexDecoration* createVertexDecoration(const Json& json) {
    const string type = json.at("type").get<string>();
    if (type == "place object") {
        return new PlaceObjectDecoration(json.at("object").get<string>());
    }
    if (type == "union") {
        return new VertexUnionDecoration();
    }
    throw runtime_error("Unknown vertex decoration type: " + type);
}

EdgeDecoration* createEdgeDecoration(const Json& json) {
    const string type = json.at("type").get<string>();
    if (type == "union") {
        return new EdgeUnionDecoration();
    }
    throw runtime_error("Unknown edge decoration type: " + type);
}

FaceDecoration* createFaceDecoration(const Json& json) {
    const string type = json.at("type").get<string>();
    if (type == "union") {
        return new FaceUnionDecoration();
    }
    throw runtime_error("Unknown face decoration type: " + type);
}

template <typename UnionType, typename Base>
void resolveUnions(
    const vector<PendingUnion>& pendingUnions,
    const map<string, Base*>& decorations,
    const string& kind
) {
    for (const auto& pending : pendingUnions) {
        auto* unionDecoration = static_cast<UnionType*>(decorations.at(pending.id));
        for (const string& childId : pending.childIds) {
            auto it = decorations.find(childId);
            if (it == decorations.end() || !it->second) {
                cerr << "Warning: Unknown " << kind << " decoration child: " << childId << endl;
                continue;
            }
            unionDecoration->addChild(it->second);
        }
    }
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

    vector<PendingUnion> pendingVertexUnions;
    vector<PendingUnion> pendingEdgeUnions;
    vector<PendingUnion> pendingFaceUnions;

    if (json.contains("vertex")) {
        for (const auto& item : json["vertex"]) {
            const string id = item.at("id").get<string>();
            result->vertexDecorations[id] = createVertexDecoration(item);
            if (item.at("type") == "union") {
                pendingVertexUnions.push_back({id, readChildIds(item)});
            }
        }
    }
    if (json.contains("edge")) {
        for (const auto& item : json["edge"]) {
            const string id = item.at("id").get<string>();
            result->edgeDecorations[id] = createEdgeDecoration(item);
            if (item.at("type") == "union") {
                pendingEdgeUnions.push_back({id, readChildIds(item)});
            }
        }
    }
    if (json.contains("face")) {
        for (const auto& item : json["face"]) {
            const string id = item.at("id").get<string>();
            result->faceDecorations[id] = createFaceDecoration(item);
            if (item.at("type") == "union") {
                pendingFaceUnions.push_back({id, readChildIds(item)});
            }
        }
    }

    resolveUnions<VertexUnionDecoration>(pendingVertexUnions, result->vertexDecorations, "vertex");
    resolveUnions<EdgeUnionDecoration>(pendingEdgeUnions, result->edgeDecorations, "edge");
    resolveUnions<FaceUnionDecoration>(pendingFaceUnions, result->faceDecorations, "face");
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
