#include "pch.h"
#include "edge_type.h"
#include "face_type.h"
#include "primitives.h"
#include "..\graph\edge_settings.h"
#include "..\util\util.h"
#include "..\util\binary_stream.h"
#include <string>
using namespace std;

std::atomic<int> EdgeType::nextId{0};

EdgeType::EdgeType(const vector<FaceData>& fData, const Vec3& direction, bool rigid)
    : faceData(fData)
    , dir(direction)
    , edgeSettings(nullptr)
    , isRigid(rigid)
    , spliced(false)
    , id(nextId++)
    , ruleGeneratorId("") {}

void EdgeType::setSpliced(bool newSpliced) {
    spliced = newSpliced;
    if (spliced) {
        edgeSettings = new EdgeSettings();
    }
}

bool EdgeType::extendable() const {
    return !isRigid;
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
    
    auto* result = new EdgeType(fData, direction, json["isRigid"]);

    // TODO: Edge Settings are often repeated. Save one copy and use an index to it.
    if (json.contains("edgeSettings") && !json["edgeSettings"].is_null()) {
        result->edgeSettings = EdgeSettings::import(json["edgeSettings"]);
    }
    // I think edge length is only needed for old grammars that have no edgeSettings.
    // result->edgeLength = json["edgeLength"].is_null() ? Util::INF : json["edgeLength"].get<double>();
    result->setSpliced(json["spliced"]);

    return result;
}

EdgeType* EdgeType::binaryDeserialize(std::istream& in, Primitives* shape) {
    Vec3 dir = bsReadVec3(in);
    bool isRigid = bsRead<uint8_t>(in) != 0;
    bool spliced = bsRead<uint8_t>(in) != 0;
    string ruleGenId  = bsReadStr(in);

    int32_t fdCount = bsRead<int32_t>(in);
    vector<FaceData> fData;
    fData.reserve(fdCount);
    for (int32_t i = 0; i < fdCount; i++) {
        int32_t ftIdx = bsRead<int32_t>(in);
        bool onRight  = bsRead<uint8_t>(in) != 0;
        fData.push_back({ shape->faceTypes[ftIdx], onRight });
    }

    auto* result = new EdgeType(fData, dir, isRigid);
    result->ruleGeneratorId = ruleGenId;

    if (spliced) {
        result->setSpliced(true); // sets result->spliced and allocates a default EdgeSettings
    }

    bool hasSettings = bsRead<uint8_t>(in) != 0;
    if (hasSettings) {
        delete result->edgeSettings;
        result->edgeSettings = new EdgeSettings();
        int32_t n;
        n = bsRead<int32_t>(in);
        for (int32_t i = 0; i < n; i++) {
            auto k = bsReadStr(in);
            double v = bsRead<double>(in);
            result->edgeSettings->set(k, v);
        }
        n = bsRead<int32_t>(in);
        for (int32_t i = 0; i < n; i++) {
            auto k = bsReadStr(in);
            bool v = bsRead<uint8_t>(in) != 0;
            result->edgeSettings->set(k, v);
        }
        n = bsRead<int32_t>(in);
        for (int32_t i = 0; i < n; i++) {
            auto k = bsReadStr(in);
            auto v = bsReadStr(in);
            result->edgeSettings->set(k, v);
        }
    }

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

const string& EdgeType::getRuleGeneratorId() const {
    return ruleGeneratorId;
}

void EdgeType::setRuleGeneratorId(const string& id) {
    ruleGeneratorId = id;
}

Json EdgeType::exportJson(const Primitives* shape) const {
    Json json;
    Json faceDataJson = Json::array();
    for (const auto& faceDatum : faceData) {
        Json faceDatumJson;
        faceDatumJson["type"] = indexOf(shape->faceTypes, faceDatum.type);
        faceDatumJson["onRight"] = faceDatum.onRight;
        faceDataJson.push_back(std::move(faceDatumJson));
    }
    json["faceData"] = faceDataJson;
    json["dir"] = dir.exportJson();
    json["isRigid"] = isRigid;
    json["spliced"] = spliced;
    if (edgeSettings) {
        json["edgeSettings"] = edgeSettings->exportJson();
    } else {
        json["edgeSettings"] = nullptr;
    }
    return json;
}
