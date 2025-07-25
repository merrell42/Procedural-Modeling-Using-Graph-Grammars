#include "pch.h"
#include "edge_type.h"
#include "face_type.h"
#include "../graph/edge_settings.h"
#include "../util/util.h"

int EdgeType::nextId = 0;

EdgeType::EdgeType(const vector<FaceData>& fData, const Vec3& direction,
                       const map<string, bool>& options)
    : faceData(fData)
    , dir(direction)
    , edgeSettings(nullptr)
    , isRigid(options.count("isRigid") ? options.at("isRigid") : false)
    , isRigidTiled(options.count("isRigidTiled") ? options.at("isRigidTiled") : false)
    , spliced(false)
    , id(nextId++) {}

void EdgeType::setSpliced(bool newSpliced) {
    spliced = newSpliced;
    if (spliced) {
        edgeSettings = new EdgeSettings();
    }
}

bool EdgeType::extendable() const {
    return !isRigid || isRigidTiled;
}

EdgeType* EdgeType::import(const Json& json, Primitives* shape) {
    vector<FaceData> fData;
    for (const auto& f : json["faceData"]) {
        fData.push_back({
            shape->faceTypes[f["type"]],
            f["onRight"]
        });
    }
    
    Vec3 direction = Vec3::import(json["dir"]);
    
    map<string, bool> options = {
        {"isRigid", json["isRigid"]},
        {"isRigidTiled", json["isRigidTiled"]}
    };
    
    auto* result = new EdgeType(fData, direction, options);

    // TODO: Edge Settings are often repeated. Save one copy and use an index to it.
    if (json.contains("edgeSettings") && !json["edgeSettings"].is_null()) {
        result->edgeSettings = EdgeSettings::import(json["edgeSettings"]);
    }
    // I think edge length is only needed for old grammars that have no edgeSettings.
    // result->edgeLength = json["edgeLength"].is_null() ? Util::INF : json["edgeLength"].get<double>();
    result->setSpliced(json["spliced"]);
    
    return result;
}

const vector<FaceData>& EdgeType::getFaceData() const {
    return faceData;
}

const Vec3& EdgeType::getDir() const {
    return dir;
}

EdgeSettings* EdgeType::getEdgeSettings() const {
    return edgeSettings;
}

bool EdgeType::getIsRigid() const {
    return isRigid;
}

bool EdgeType::getSpliced() const {
    return spliced;
}

int EdgeType::getId() const {
    return id;
}