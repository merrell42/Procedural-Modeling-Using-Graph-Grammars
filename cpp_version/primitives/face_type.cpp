#include "pch.h"
#include "face_type.h"
#include "../util/util.h"
#include "../geometry/vec2.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>

FaceType::FaceType(const string& mat, const Vec3& n)
    : material(mat)
    , normal(n) {
    maxDim = Util::maxDim(normal);
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

const Vec3& FaceType::getColor() const {
    return color;
}

int FaceType::getMaxDim() const {
    return maxDim;
}

const string& FaceType::getSignature() const {
    return signature;
}

void FaceType::setRuleGeneratorData(const string& sig) {
    signature = sig;
}

FaceType* FaceType::importRuleGenerator(const Json& json) {
    string material = "";
    if (json.contains("material") && json["material"].is_string()) {
        material = json["material"].get<string>();
    }
    auto normal = json.contains("normal") ?
        Vec3::import(json["normal"]) : Vec3(0, 0, 1);
    auto* result = new FaceType(material, normal);
    
    if (json.contains("color") && json["color"] != nullptr) {
        result->color = Vec3::import(json["color"]);
    }
    
    // Set RuleGenerator-specific data.
    if (json.contains("signature")) {
        result->signature = json["signature"].get<string>();
    }
    
    return result;
}