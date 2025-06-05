#include "pch.h"
#include "face_type.h"
#include "../util/util.h"
#include "../geometry/vec2.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>

namespace ms {

int FaceType::nextId = 0;

FaceType::FaceType(const string& mat, const Vec3& n)
    : material(mat)
    , normal(n)
    , monotonic(false)
    , color(nullptr)
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

double FaceType::angle(const Vec3& q) const {
    double dx = u.dot(q);
    double dy = v.dot(q);
    double angle = atan2(dy, dx);
    
    // Angles near pi wrap to -pi.
    if (angle > M_PI - EPS) {
        return -angle;
    }
    return angle;
}

double FaceType::getArea(const vector<Vec3>& vertices) const {
    vector<Vec2> projectedVertices;
    for (const auto& vertex : vertices) {
        projectedVertices.push_back({u.dot(vertex), v.dot(vertex)});
    }
    return -polygonArea(projectedVertices);
}

double FaceType::polygonArea(const vector<Vec2>& points) const {
    double area = 0;
    for (size_t i = 0; i < points.size(); i++) {
        const auto& p1 = points[i];
        const auto& p2 = points[(i + 1) % points.size()];
        area += p1.getX() * p2.getY() - p2.getX() * p1.getY();
    }
    return area / 2;
}

Vec3 FaceType::normalColor() const {
    if (color) {
        return *color;
    }
    return Vec3(
        0.5f * normal.getX() + 0.5f,
        0.5f * normal.getY() + 0.5f,
        0.5f * normal.getZ() + 0.5f
    );
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
        result->color = new Vec3(Vec3::import(json["color"]));
    }
    
    return result;
}

} // namespace ms 