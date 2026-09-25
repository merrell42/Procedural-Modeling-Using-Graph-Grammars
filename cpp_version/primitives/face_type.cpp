#include "pch.h"
#include "face_type.h"
#include "primitives.h"
#include "../decorations/decorations.h"
#include "../decorations/face_decoration.h"
#include "../util/util.h"
#include "../util/binary_stream.h"
#include "../geometry/vec2.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>
#include <iostream>

static constexpr double EPS = 1e-5;

FaceType::FaceType(const string& mat, const Vec3& n)
    : material(mat)
    , decoration(nullptr)
    , normal(n) {
    maxDim = Util::maxDim(normal);
    Vec3::orthonormalBasis(normal, u, v);
    color = Vec3(1, 1, 1);
}

double FaceType::angle(const Vec3& dir) const {
    double dx = u.dot(dir);
    double dy = v.dot(dir);
    double result = atan2(dy, dx);
    // Angles near pi wrap to -pi.
    if (result > M_PI - EPS) {
        return -result;
    }
    return result;
}


FaceType* FaceType::import(const Json& json, Primitives* shape) {
    string material = "";
    if (json.contains("material") && json["material"].is_string()) {
        material = json["material"].get<string>();
    }
    auto normal = json.contains("normal") ?
        Vec3::import(json["normal"]) : Vec3(0, 0, 1);
    auto* result = new FaceType(material, normal);
    
    if (json.contains("color") && !json["color"].is_null()) {
        result->color = Vec3::import(json["color"]);
    }
    if (json.contains("decoration") && json["decoration"].is_string()) {
        const string id = json["decoration"].get<string>();
        result->decoration = shape->getDecorations()->getFaceDecoration(id);
        if (!result->decoration) {
            cerr << "Warning: Unknown face decoration: " << id << endl;
        }
    }
    
    return result;
}

FaceType* FaceType::binaryDeserialize(std::istream& in) {
    string material = bsReadStr(in);
    Vec3 normal = bsReadVec3(in);
    Vec3 color = bsReadVec3(in);
    auto* result = new FaceType(material, normal);
    result->color = color;
    return result;
}

const string& FaceType::getMaterial() const {
    return material;
}

FaceDecoration* FaceType::getDecoration() const {
    return decoration;
}

void FaceType::setDecoration(FaceDecoration* decoration) {
    this->decoration = decoration;
}

const Vec3& FaceType::getNormal() const {
    return normal;
}

const Vec3& FaceType::getColor() const {
    return color;
}

int FaceType::getMaxDim() const {
    return maxDim;
}

Json FaceType::exportJson(const Primitives* shape) const {
    Json json;
    json["material"] = material;
    if (decoration) {
        json["decoration"] = decoration->getId();
    }
    json["normal"] = normal.exportJson();
    json["color"] = color.exportJson();
    return json;
}
