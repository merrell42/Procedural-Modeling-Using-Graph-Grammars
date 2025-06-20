#include "pch.h"
#include "face_type.h"
#include "../util/util.h"
#include "../geometry/vec2.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>

int FaceType::nextId = 0;

FaceType::FaceType(const string& mat, const Vec3& n)
    : material(mat)
    , normal(n)
    , id(nextId++) {
    computeOrthonormalBasis();
    maxDim = Util::maxDim(normal);
}

void FaceType::computeOrthonormalBasis() {
    // Find the smallest component of the normal.
    int minDim = 0;
    double minVal = abs(normal[0]);
    for (int i = 1; i < 3; i++) {
        if (abs(normal[i]) < minVal) {
            minDim = i;
            minVal = abs(normal[i]);
        }
    }
    // Create two vectors orthogonal to the normal.
    u = Vec3();
    u.setValue(1.0f, minDim);
    u = normal.cross(u).normalize();
    v = normal.cross(u).normalize();
}


FaceType* FaceType::import(const Json& json) {
    string material = "";
    if (json.contains("material") && json["material"].is_string()) {
        material = json["material"].get<string>();
    }
    auto normal = json.contains("normal") ?
        Vec3::import(json["normal"]) : Vec3(0, 0, 1);
    auto* result = new FaceType(material, normal);
    
    if (json["color"] != nullptr) {
        result->color = Vec3::import(json["color"]);
    }
    
    return result;
}

const string& FaceType::getMaterial() const {
    return material;
}

const Vec3& FaceType::getNormal() const {
    return normal;
}

int FaceType::getMaxDim() const {
    return maxDim;
}
