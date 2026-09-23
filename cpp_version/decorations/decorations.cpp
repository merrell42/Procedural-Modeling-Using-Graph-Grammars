#include "pch.h"
#include "decorations.h"
#include "place_object_decoration.h"
#include "union_decoration.h"
#include "pick_random_decoration.h"
#include <iostream>
#include <stdexcept>

using namespace std;

namespace {

struct PendingChildren {
    string id;
    vector<string> childIds;
    vector<double> weights;
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

vector<double> readWeights(const Json& json) {
    vector<double> weights;
    if (!json.contains("weight") || !json["weight"].is_array()) {
        return weights;
    }
    for (const auto& weight : json["weight"]) {
        weights.push_back(weight.get<double>());
    }
    return weights;
}

PendingChildren readPendingChildren(const Json& json) {
    return {
        json.at("id").get<string>(),
        readChildIds(json),
        readWeights(json)
    };
}

VertexDecoration* createVertexDecoration(const Json& json) {
    const string type = json.at("type").get<string>();
    if (type == "place object") {
        return new PlaceObjectDecoration(json.at("object").get<string>());
    }
    if (type == "union") {
        return new VertexUnionDecoration();
    }
    if (type == "pick random") {
        return new VertexPickRandomDecoration();
    }
    throw runtime_error("Unknown vertex decoration type: " + type);
}

EdgeDecoration* createEdgeDecoration(const Json& json) {
    const string type = json.at("type").get<string>();
    if (type == "union") {
        return new EdgeUnionDecoration();
    }
    if (type == "pick random") {
        return new EdgePickRandomDecoration();
    }
    throw runtime_error("Unknown edge decoration type: " + type);
}

FaceDecoration* createFaceDecoration(const Json& json) {
    const string type = json.at("type").get<string>();
    if (type == "union") {
        return new FaceUnionDecoration();
    }
    if (type == "pick random") {
        return new FacePickRandomDecoration();
    }
    throw runtime_error("Unknown face decoration type: " + type);
}

template <typename Base>
Base* findChildOrWarn(
    const map<string, Base*>& decorations,
    const string& childId,
    const string& kind
) {
    auto it = decorations.find(childId);
    if (it == decorations.end() || !it->second) {
        cerr << "Warning: Unknown " << kind << " decoration child: " << childId << endl;
        return nullptr;
    }
    return it->second;
}

template <typename UnionType, typename Base>
void resolveUnions(
    const vector<PendingChildren>& pendingList,
    const map<string, Base*>& decorations,
    const string& kind
) {
    for (const auto& pending : pendingList) {
        auto* parent = static_cast<UnionType*>(decorations.at(pending.id));
        for (const string& childId : pending.childIds) {
            if (Base* child = findChildOrWarn(decorations, childId, kind)) {
                parent->addChild(child);
            }
        }
    }
}

template <typename PickType, typename Base>
void resolvePickRandom(
    const vector<PendingChildren>& pendingList,
    const map<string, Base*>& decorations,
    const string& kind
) {
    for (const auto& pending : pendingList) {
        auto* parent = static_cast<PickType*>(decorations.at(pending.id));
        parent->setUseWeights(!pending.weights.empty());
        for (size_t i = 0; i < pending.childIds.size(); i++) {
            Base* child = findChildOrWarn(decorations, pending.childIds[i], kind);
            if (!child) {
                continue;
            }
            double weight = i < pending.weights.size() ? pending.weights[i] : 1.0;
            parent->addChild(child, weight);
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

    vector<PendingChildren> pendingVertexUnions;
    vector<PendingChildren> pendingEdgeUnions;
    vector<PendingChildren> pendingFaceUnions;
    vector<PendingChildren> pendingVertexPicks;
    vector<PendingChildren> pendingEdgePicks;
    vector<PendingChildren> pendingFacePicks;

    if (json.contains("vertex")) {
        for (const auto& item : json["vertex"]) {
            const string type = item.at("type").get<string>();
            result->vertexDecorations[item.at("id").get<string>()] = createVertexDecoration(item);
            if (type == "union") {
                pendingVertexUnions.push_back(readPendingChildren(item));
            } else if (type == "pick random") {
                pendingVertexPicks.push_back(readPendingChildren(item));
            }
        }
    }
    if (json.contains("edge")) {
        for (const auto& item : json["edge"]) {
            const string type = item.at("type").get<string>();
            result->edgeDecorations[item.at("id").get<string>()] = createEdgeDecoration(item);
            if (type == "union") {
                pendingEdgeUnions.push_back(readPendingChildren(item));
            } else if (type == "pick random") {
                pendingEdgePicks.push_back(readPendingChildren(item));
            }
        }
    }
    if (json.contains("face")) {
        for (const auto& item : json["face"]) {
            const string type = item.at("type").get<string>();
            result->faceDecorations[item.at("id").get<string>()] = createFaceDecoration(item);
            if (type == "union") {
                pendingFaceUnions.push_back(readPendingChildren(item));
            } else if (type == "pick random") {
                pendingFacePicks.push_back(readPendingChildren(item));
            }
        }
    }

    resolveUnions<VertexUnionDecoration>(pendingVertexUnions, result->vertexDecorations, "vertex");
    resolveUnions<EdgeUnionDecoration>(pendingEdgeUnions, result->edgeDecorations, "edge");
    resolveUnions<FaceUnionDecoration>(pendingFaceUnions, result->faceDecorations, "face");
    resolvePickRandom<VertexPickRandomDecoration>(pendingVertexPicks, result->vertexDecorations, "vertex");
    resolvePickRandom<EdgePickRandomDecoration>(pendingEdgePicks, result->edgeDecorations, "edge");
    resolvePickRandom<FacePickRandomDecoration>(pendingFacePicks, result->faceDecorations, "face");
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
