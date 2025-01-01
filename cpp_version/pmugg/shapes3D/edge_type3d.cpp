#include "edge_type3d.h"
#include "face_type3d.h"
#include "..\decoration\brush.h"
#include "..\util\util.h"

namespace ms {

int EdgeType3D::nextId = 0;

EdgeType3D::EdgeType3D(const std::vector<FaceData>& fData, const Vec3& direction,
                       const std::map<std::string, bool>& options)
    : faceData(fData)
    , dir(direction)
    , brush(nullptr)
    , angle(std::atan2(dir.getY(), dir.getX()))
    , edgeLength(INFINITY)
    , offset(nullptr)
    , isRigid(options.count("isRigid") ? options.at("isRigid") : false)
    , isRigidTiled(options.count("isRigidTiled") ? options.at("isRigidTiled") : false)
    , monotonic(false)
    , spliced(false)
    , destroyed(false)
    , id(nextId++) {}

void EdgeType3D::setSpliced(bool newSpliced) {
    spliced = newSpliced;
    if (spliced) {
        brush = new Brush("#aaa");
    }
}

bool EdgeType3D::isLoopy() const {
    return brush ? brush->getBool("Loopy") : true;
}

bool EdgeType3D::isBoundary() const {
    return brush ? brush->getBool("Boundary") : false;
}

bool EdgeType3D::isConnected() const {
    if (spliced) return false;
    return brush ? brush->getBool("Fully Connected") : false;
}

bool EdgeType3D::singleFragment() const {
    return isBoundary();
}

bool EdgeType3D::splittable() const {
    return !(!isLoopy() || isBoundary() || isConnected());
}

bool EdgeType3D::extendable() const {
    return !isRigid || isRigidTiled;
}

float EdgeType3D::getThickness() const {
    return brush ? brush->getFloat("Thickness") : 1.0f;
}

std::string EdgeType3D::boundaryString() const {
    if (dir.dot(Vec3::X_AXIS) > 0.99f) return "x";
    if (dir.dot(Vec3::Y_AXIS) > 0.99f) return "y";
    if (dir.dot(Vec3::Z_AXIS) > 0.99f) return "z";
    return std::string(1, static_cast<char>(id + 'a'));
}

int EdgeType3D::neighboringFace(int initialIndex, bool above) const {
    int maxDim = Util::maxDim(dir);
    std::vector<std::pair<float, int>> angles;
    
    for (size_t i = 0; i < faceData.size(); i++) {
        const auto& f = faceData[i];
        Vec3 v = dir.cross(f.type->getNormal());
        if (!f.onRight) {
            v.scale(-1);
        }
        
        float x = 0;
        float y = 0;
        switch(maxDim) {
            case 0: x = v.getX(); y = v.getY(); break;
            case 1: x = v.getZ(); y = v.getX(); break;
            case 2: x = v.getY(); y = v.getZ(); break;
        }
        angles.push_back({std::atan2(x, y), i});
    }
    
    std::sort(angles.begin(), angles.end());
    auto fOrder = std::find_if(angles.begin(), angles.end(),
        [initialIndex](const auto& p) { return p.second == initialIndex; }) - angles.begin();
    
    int neighborOrder = (int)(fOrder + (above ? 1 : -1) + angles.size()) % angles.size();
    return angles[neighborOrder].second;
}

std::string EdgeType3D::getLeftArea() const {
    auto it = std::find_if(faceData.begin(), faceData.end(),
        [](const FaceData& f) { return !f.onRight; });
    return it != faceData.end() ? it->type->getMaterial() : "";
}

std::string EdgeType3D::getRightArea() const {
    auto it = std::find_if(faceData.begin(), faceData.end(),
        [](const FaceData& f) { return f.onRight; });
    return it != faceData.end() ? it->type->getMaterial() : "";
}

/* Json EdgeType3D::export(const Types& types) const {
    Json json;
    
    json["faceData"] = Json::array();
    for (const auto& f : faceData) {
        json["faceData"].push_back({
            {"type", types.faceTypes.indexOf(f.type)},
            {"onRight", f.onRight}
        });
    }
    
    if (brush && brush->export_) {
        json["brush"] = brush->export_();
    }
    
    json["dir"] = dir.export();
    json["isRigid"] = isRigid;
    json["isRigidTiled"] = isRigidTiled;
    json["edgeLength"] = edgeLength;
    json["angle"] = angle;
    json["offset"] = offset ? offset->export() : nullptr;
    json["spliced"] = spliced;
    
    return json;
} */

EdgeType3D* EdgeType3D::import(const Json& json, Shape3D* shape) {
    std::vector<FaceData> fData;
    for (const auto& f : json["faceData"]) {
        fData.push_back({
            shape->faceTypes[f["type"]],
            f["onRight"]
        });
    }
    
    Vec3 direction = Vec3::import(json["dir"]);
    
    std::map<std::string, bool> options = {
        {"isRigid", json["isRigid"]},
        {"isRigidTiled", json["isRigidTiled"]}
    };
    
    auto* result = new EdgeType3D(fData, direction, options);
    
    if (json.contains("brush")) {
        result->brush = Brush::import(json["brush"]);
    }
    // I think edge length is only needed for old grammars that have no brushes.
    // result->edgeLength = json["edgeLength"].is_null() ? Util::INF : json["edgeLength"].get<float>();
    /*result->angle = json.contains("angle") ?
        json["angle"].get<float>() :
        Vec2::angle(Vec2::ORIGIN, result->dir);*/
    if (json["offset"] != nullptr) {
        result->offset = new Vec3(Vec3::import(json["offset"]));
    }
    result->setSpliced(json["spliced"]);
    
    return result;
}

EdgeType3D* EdgeType3D::partialImport(const Json& json, const std::vector<FaceType3D*>& faceTypes) {
    std::vector<FaceData> fData;
    for (const auto& f : json["faceData"]) {
        fData.push_back({
            faceTypes[f["type"]],
            f["onRight"]
        });
    }
    
    Vec3 direction = Vec3::import(json["dir"]).swapAxes();
    auto* brush = new Brush("#000", "#000");
    
    auto* result = new EdgeType3D(fData, direction, {{"brush", brush}});
    result->id = json["id"];
    
    return result;
}

} // namespace ms 