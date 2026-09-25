#include "pch.h"
#include "decorations.h"
#include "decorations_factory.h"
#include <stdexcept>

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
void resolveAll(const Decorations& decorations, const map<string, Base*>& decorationsMap) {
    for (auto& entry : decorationsMap) {
        if (entry.second) {
            entry.second->resolveChildren(decorations);
        }
    }
}

enum class VisitState {
    Unseen,
    Visiting,
    Visited
};

template <typename Base>
void visitDecoration(
    const Base* decoration,
    vector<const Base*>& path,
    map<const Base*, VisitState>& state
) {
    if (!decoration) {
        return;
    }
    VisitState& visitState = state[decoration];
    if (visitState == VisitState::Visited) {
        return;
    }
    if (visitState == VisitState::Visiting) {
        string cycle;
        for (const Base* node : path) {
            if (cycle.empty() && node != decoration) {
                continue;
            }
            if (!cycle.empty()) {
                cycle += " -> ";
            }
            cycle += node->getId();
        }
        cycle += " -> ";
        cycle += decoration->getId();
        throw runtime_error("Circular decoration dependency: " + cycle);
    }
    visitState = VisitState::Visiting;
    path.push_back(decoration);
    for (Base* child : decoration->getChildren()) {
        visitDecoration(child, path, state);
    }
    path.pop_back();
    visitState = VisitState::Visited;
}

template <typename Base>
void checkForCycles(const map<string, Base*>& decorations) {
    map<const Base*, VisitState> state;
    for (const auto& entry : decorations) {
        vector<const Base*> path;
        visitDecoration(entry.second, path, state);
    }
}

template <typename Base>
void importKind(const Json& items, map<string, Base*>& decorations, Base* (*create)(const Json&)) {
    for (const auto& item : items) {
        Base* decoration = create(item);
        if (!decoration) {
            continue;
        }
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

Decorations* Decorations::import(const Json& json, const string& assetDirectory) {
    DecorationsFactory::setAssetDirectory(assetDirectory);
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

    try {
        resolveAll(*result, result->vertexDecorations);
        resolveAll(*result, result->edgeDecorations);
        resolveAll(*result, result->faceDecorations);
        checkForCycles(result->vertexDecorations);
        checkForCycles(result->edgeDecorations);
        checkForCycles(result->faceDecorations);
    } catch (...) {
        delete result;
        throw;
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
