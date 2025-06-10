#include "pch.h"
#include "edge_type.h"
#include "face_type.h"
#include "..\decoration\brush.h"
#include "..\util\util.h"



int EdgeType::nextId = 0;

EdgeType::EdgeType(const vector<FaceData>& fData, const Vec3& direction,
                       const map<string, bool>& options)
    : faceData(fData)
    , dir(direction)
    , brush(nullptr)
    , angle(atan2(dir.getY(), dir.getX()))
    , edgeLength(INFINITY)
    , offset(nullptr)
    , isRigid(options.count("isRigid") ? options.at("isRigid") : false)
    , isRigidTiled(options.count("isRigidTiled") ? options.at("isRigidTiled") : false)
    , monotonic(false)
    , spliced(false)
    , id(nextId++) {}

void EdgeType::setSpliced(bool newSpliced) {
    spliced = newSpliced;
    if (spliced) {
        brush = new Brush();
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
    
    if (json.contains("brush") && !json["brush"].is_null()) {
        result->brush = Brush::import(json["brush"]);
    }
    // I think edge length is only needed for old grammars that have no brushes.
    // result->edgeLength = json["edgeLength"].is_null() ? Util::INF : json["edgeLength"].get<double>();
    /*result->angle = json.contains("angle") ?
        json["angle"].get<double>() :
        Vec2::angle(Vec2::ORIGIN, result->dir);*/
    if (json["offset"] != nullptr) {
        result->offset = new Vec3(Vec3::import(json["offset"]));
    }
    result->setSpliced(json["spliced"]);
    
    return result;
}

const vector<FaceData>& EdgeType::getFaceData() const {
    return faceData;
}

const Vec3& EdgeType::getDir() const {
    return dir;
}

Brush* EdgeType::getBrush() const {
    return brush;
}

double EdgeType::getEdgeLength() const {
    return edgeLength;
}

bool EdgeType::getIsRigid() const {
    return isRigid;
}

bool EdgeType::getSpliced() const {
    return spliced;
}

double EdgeType::getAngle() const {
    return angle;
}

int EdgeType::getId() const {
    return id;
}