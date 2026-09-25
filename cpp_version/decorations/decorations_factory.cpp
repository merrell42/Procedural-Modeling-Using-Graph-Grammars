#include "pch.h"
#include "decorations_factory.h"
#include "place_object_decoration.h"
#include "rotate_decoration.h"
#include "scale_decoration.h"
#include "space_evenly_decoration.h"
#include "space_randomly_decoration.h"
#include "scatter_decoration.h"
#include "union_decoration.h"
#include "pick_random_decoration.h"
#include "pending_decoration.h"
#include "../geometry/vec3.h"
#include <iostream>
#include <stdexcept>

using namespace std;

namespace {

vector<string> readChildIds(const Json& json) {
    vector<string> childIds;
    if (json.contains("childIds") && json["childIds"].is_array()) {
        for (const auto& childId : json["childIds"]) {
            childIds.push_back(childId.get<string>());
        }
    }
    return childIds;
}

vector<double> readWeights(const Json& json) {
    vector<double> weights;
    if (json.contains("weight") && json["weight"].is_array()) {
        for (const auto& weight : json["weight"]) {
            weights.push_back(weight.get<double>());
        }
    }
    return weights;
}

template <typename UnionType, typename PickType, typename PendingType, typename Base>
Base* createCompositeDecoration(const Json& json, const string& type) {
    if (type == "union") {
        auto* decoration = new UnionType();
        for (const string& childId : readChildIds(json)) {
            decoration->addChild(new PendingType(childId));
        }
        return decoration;
    }
    if (type == "pick random") {
        auto* decoration = new PickType();
        const auto childIds = readChildIds(json);
        const auto weights = readWeights(json);
        if (!weights.empty() && weights.size() != childIds.size()) {
            cerr << "Error: pick random decoration \"" << json.at("id").get<string>()
                 << "\" has " << weights.size() << " weights and "
                 << childIds.size() << " childIds" << endl;
        }
        decoration->setWeights(weights);
        for (const string& childId : childIds) {
            decoration->addChild(new PendingType(childId));
        }
        return decoration;
    }
    return nullptr;
}

} // namespace

VertexDecoration* DecorationsFactory::createVertexDecoration(const Json& json) {
    const string type = json.at("type").get<string>();
    if (type == "place object") {
        return new PlaceObjectDecoration(json.at("object").get<string>());
    }
    if (type == "rotate") {
        Vec3 axis(0, 0, 1);
        if (json.contains("axis")) {
            axis = Vec3::import(json.at("axis"));
        }
        auto* decoration = new RotateDecoration(
            json.at("minAngle").get<double>(),
            json.at("maxAngle").get<double>(),
            axis
        );
        decoration->setChild(new VertexPendingDecoration(json["child"].get<string>()));
        return decoration;
    }
    if (type == "scale") {
        const Json& minJson = json.at("min");
        const Json& maxJson = json.at("max");
        Vec3 minScale;
        Vec3 maxScale;
        bool uniform = !minJson.is_array();
        if (uniform) {
            double minValue = minJson.get<double>();
            double maxValue = maxJson.get<double>();
            minScale = Vec3(minValue, minValue, minValue);
            maxScale = Vec3(maxValue, maxValue, maxValue);
        } else {
            minScale = Vec3::import(minJson);
            maxScale = Vec3::import(maxJson);
        }
        auto* decoration = new ScaleDecoration(minScale, maxScale, uniform);
        decoration->setChild(new VertexPendingDecoration(json["child"].get<string>()));
        return decoration;
    }

    VertexDecoration* decoration = createCompositeDecoration<
        VertexUnionDecoration,
        VertexPickRandomDecoration,
        VertexPendingDecoration,
        VertexDecoration
    >(json, type);
    if (decoration) {
        return decoration;
    }
    throw runtime_error("Unknown vertex decoration type: " + type);
}

EdgeDecoration* DecorationsFactory::createEdgeDecoration(const Json& json) {
    const string type = json.at("type").get<string>();
    if (type == "space evenly") {
        auto* decoration = new SpaceEvenlyDecoration(json.at("spacing").get<double>());
        decoration->setChild(new VertexPendingDecoration(json["child"].get<string>()));
        return decoration;
    }
    if (type == "space randomly") {
        auto* decoration = new SpaceRandomlyDecoration(json.at("spacing").get<double>());
        decoration->setChild(new VertexPendingDecoration(json["child"].get<string>()));
        return decoration;
    }

    EdgeDecoration* decoration = createCompositeDecoration<
        EdgeUnionDecoration,
        EdgePickRandomDecoration,
        EdgePendingDecoration,
        EdgeDecoration
    >(json, type);
    if (decoration) {
        return decoration;
    }
    throw runtime_error("Unknown edge decoration type: " + type);
}

FaceDecoration* DecorationsFactory::createFaceDecoration(const Json& json) {
    const string type = json.at("type").get<string>();
    if (type == "scatter") {
        auto* decoration = new ScatterDecoration(json.at("density").get<double>());
        decoration->setChild(new VertexPendingDecoration(json["child"].get<string>()));
        return decoration;
    }

    FaceDecoration* decoration = createCompositeDecoration<
        FaceUnionDecoration,
        FacePickRandomDecoration,
        FacePendingDecoration,
        FaceDecoration
    >(json, type);
    if (decoration) {
        return decoration;
    }
    throw runtime_error("Unknown face decoration type: " + type);
}
