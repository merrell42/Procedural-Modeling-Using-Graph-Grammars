#include "pch.h"
#include "scatter_decoration.h"
#include "pending_decoration.h"
#include "../graph_drawing/face.h"
#include "../geometry/matrix4.h"
#include "../util/util.h"
#include <algorithm>
#include <cmath>

namespace {

double triangleArea(const Vec3& a, const Vec3& b, const Vec3& c) {
    return (b - a).cross(c - a).length() * 0.5;
}

Vec3 randomPointInTriangle(const Vec3& a, const Vec3& b, const Vec3& c) {
    double r1 = sqrt(randomValue());
    double r2 = randomValue();
    return a * (1.0 - r1) + b * (r1 * (1.0 - r2)) + c * (r1 * r2);
}

} // namespace

ScatterDecoration::ScatterDecoration(double density) : density(density) {}

void ScatterDecoration::setChild(VertexDecoration* child) {
    this->child = child;
}

void ScatterDecoration::resolveChildren(const Decorations& decorations) {
    vector<VertexDecoration*> children = { child };
    resolvePendingChildren<VertexDecoration, const Matrix4&>(children, decorations);
    child = children[0];
}

struct Triangle {
    Vec3 a, b, c; 
    double area;
};

DecorationOutput ScatterDecoration::getOutput(const Face& face) const {
    if (!child || density <= 0.0 || face.isHole()) {
        return {};
    }

    const vector<Vec3> positions = face.getPositions();
    const vector<int>& indices = face.getTriangleIndices();
    vector<Triangle> triangles;
    vector<double> cumulativeAreas;
    double totalArea = 0.0;
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        int i0 = indices[i];
        int i1 = indices[i + 1];
        int i2 = indices[i + 2];
        Triangle triangle;
        triangle.a = positions[i0];
        triangle.b = positions[i1];
        triangle.c = positions[i2];
        triangle.area = triangleArea(triangle.a, triangle.b, triangle.c);
        if (triangle.area <= 0.0) {
            continue;
        }
        totalArea += triangle.area;
        cumulativeAreas.push_back(totalArea);
        triangles.push_back(triangle);
    }
    if (triangles.empty()) {
        return {};
    }

    int count = Util::randomPoisson(density * totalArea);
    DecorationOutput output;
    for (int n = 0; n < count; n++) {
        double target = randomValue() * totalArea;
        auto it = lower_bound(cumulativeAreas.begin(), cumulativeAreas.end(), target);
        size_t index = (size_t)distance(cumulativeAreas.begin(), it);
        if (index >= triangles.size()) {
            index = triangles.size() - 1;
        }
        const Triangle& triangle = triangles[index];
        Vec3 position = randomPointInTriangle(triangle.a, triangle.b, triangle.c);
        output.append(child->getOutput(Matrix4::translation(position)));
    }
    return output;
}
