#include "pch.h"
#include "face_type3d.h"
// #include "graph.h"
// #include "endpoint.h"
#include "../util/util.h"
#include "../shape/vec2.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>

namespace ms {

int FaceType3D::nextId = 0;

FaceType3D::FaceType3D(const std::string& mat, const Vec3& n)
    : material(mat)
    , normal(n)
    , monotonic(false)
    , color(nullptr)
    , id(nextId++) {
    computeOrthonormalBasis();
    maxDim = Util::maxDim(normal);
}

void FaceType3D::computeOrthonormalBasis() {
    // Find the least significant component of the normal
    int minDim = 0;
    float minVal = std::abs(normal[0]);
    for (int i = 1; i < 3; i++) {
        if (std::abs(normal[i]) < minVal) {
            minDim = i;
            minVal = std::abs(normal[i]);
        }
    }

    // Create a vector orthogonal to the normal
    u = Vec3();
    u.setValue(1.0f, minDim);
    u = normal.cross(u).normalize();

    // Complete the orthonormal basis
    v = normal.cross(u).normalize();
}

float FaceType3D::angle(const Vec3& q) const {
    float dx = u.dot(q);
    float dy = v.dot(q);
    float angle = std::atan2(dy, dx);
    
    // Angles near pi wrap to -pi
    if (angle > M_PI - EPS) {
        return -angle;
    }
    return angle;
}

float FaceType3D::getArea(const std::vector<Vec3>& vertices) const {
    std::vector<Vec2> projectedVertices;
    for (const auto& vertex : vertices) {
        projectedVertices.push_back({u.dot(vertex), v.dot(vertex)});
    }
    return -polygonArea(projectedVertices);
}

float FaceType3D::polygonArea(const std::vector<Vec2>& points) const {
    float area = 0;
    for (size_t i = 0; i < points.size(); i++) {
        const auto& p1 = points[i];
        const auto& p2 = points[(i + 1) % points.size()];
        area += p1.getX() * p2.getY() - p2.getX() * p1.getY();
    }
    return area / 2;
}

//void FaceType3D::computeMonotonic(const std::vector<Graph*>& graphs) {
//    bool positive = false;
//    bool negative = false;
//
//    for (const auto* graph : graphs) {
//        for (const auto* endpoint : graph->getOuterEndpoints()) {
//            if (endpoint->getFaceTypes().contains(this) && endpoint->oriented(this)) {
//                auto trace = graph->traceToExit3D(endpoint, this);
//                if (trace.winding > 0) {
//                    positive = true;
//                }
//                if (trace.winding < 0) {
//                    negative = true;
//                }
//            }
//        }
//    }
//
//    monotonic = positive ^ negative;  // XOR operation
//}

Vec3 FaceType3D::normalColor() const {
    if (color) {
        return *color;
    }
    return Vec3(
        0.5f * normal.getX() + 0.5f,
        0.5f * normal.getY() + 0.5f,
        0.5f * normal.getZ() + 0.5f
    );
}

/* Json FaceType3D::export() const {
    Json json;
    json["color"] = color ? color->export() : nullptr;
    json["material"] = material;
    json["normal"] = normal.export();
    return json;
} */

FaceType3D* FaceType3D::import(const Json& json) {
    std::string material = "";
    if (json.contains("material") && json["material"].is_string()) {
        material = json["material"].get<std::string>();
    }
    auto normal = json.contains("normal") ?
        Vec3::import(json["normal"]) : Vec3(0, 0, 1);
    auto* result = new FaceType3D(material, normal);
    
    if (json["color"] != nullptr) {
        result->color = new Vec3(Vec3::import(json["color"]));
    }
    
    return result;
}

FaceType3D* FaceType3D::partialImport(const Json& json) {
    auto* result = new FaceType3D(
        "",  // Empty material for partial import
        Vec3::import(json["normal"]).swapAxes()
    );
    result->id = json["signature"];
    return result;
}

} // namespace ms 