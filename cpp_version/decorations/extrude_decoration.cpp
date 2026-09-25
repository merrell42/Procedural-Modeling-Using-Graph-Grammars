#include "pch.h"
#include "extrude_decoration.h"
#include "../third_party/earcut/earcut.h"
#include "../util/util.h"
#include <algorithm>
#include <array>

namespace {

constexpr double EPS = 0.001;

void addQuad(Extrusion& extrusion, const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d) {
    Vec3 normal = (b - a).cross(d - a);
    normal.normalize();
    extrusion.positions.push_back(a);
    extrusion.positions.push_back(b);
    extrusion.positions.push_back(c);
    extrusion.positions.push_back(d);
    for (int i = 0; i < 4; i++) {
        extrusion.normals.push_back(normal);
    }
}

vector<Vec2> orientedProfile(const vector<Vec2>& polygon) {
    vector<Vec2> profile = polygon;
    double area = Util::signedArea(profile);
    if (area < 0.0) {
        reverse(profile.begin(), profile.end());
    } else if (area == 0.0) {
        return {};
    }
    return profile;
}

vector<Vec2> scaledProfile(const vector<Vec2>& polygon, double scale) {
    vector<Vec2> scaled;
    scaled.reserve(polygon.size());
    for (const Vec2& point : polygon) {
        scaled.push_back(point * scale);
    }
    return orientedProfile(scaled);
}

} // namespace

ExtrudeDecoration::ExtrudeDecoration(const vector<Vec2>& polygon, const Vec3& color, double scale)
    : profile(scaledProfile(polygon, scale)),
      color{ (float)color.getX(), (float)color.getY(), (float)color.getZ() } {
    using Point = array<double, 2>;
    vector<vector<Point>> polygons(1);
    for (const Vec2& point : profile) {
        polygons[0].push_back({ point.x, point.y });
    }

    // Precompute the triangle indices for the sides.
    int count = (int)profile.size();
    for (int i = 0; i < count; i++) {
        int base = i * 4;
        triangles.push_back(base);
        triangles.push_back(base + 2);
        triangles.push_back(base + 1);
        triangles.push_back(base);
        triangles.push_back(base + 3);
        triangles.push_back(base + 2);
    }
    
    auto capIndices = mapbox::earcut(polygons);
    if (capIndices.size() < 3) {
        return;
    }

    // Precompute the triangle indices for the caps.
    includeCaps = true;
    int endBase = count * 4;
    int startBase = endBase + count;
    for (size_t i = 0; i + 2 < capIndices.size(); i += 3) {
        triangles.push_back(endBase + (int)capIndices[i]);
        triangles.push_back(endBase + (int)capIndices[i + 2]);
        triangles.push_back(endBase + (int)capIndices[i + 1]);
    }
    for (size_t i = 0; i + 2 < capIndices.size(); i += 3) {
        triangles.push_back(startBase + (int)capIndices[i]);
        triangles.push_back(startBase + (int)capIndices[i + 1]);
        triangles.push_back(startBase + (int)capIndices[i + 2]);
    }
}

DecorationOutput ExtrudeDecoration::getOutput(const Vec3& start, const Vec3& end) const {
    Vec3 side, up;
    Vec3 tangent = end - start;
    double length = tangent.length();
    tangent.normalize();
    Vec3::orthonormalBasis(tangent, side, up);

    // Trim the endpoint to prevent z-fighting.
    Vec3 trimmedStart = start;
    Vec3 trimmedEnd = end;
    if (length > EPS * 2.0) {
        trimmedStart = start + tangent * EPS;
        trimmedEnd = end - tangent * EPS;
    }

    Extrusion extrusion;
    extrusion.color = color;
    extrusion.triangles = triangles;
    size_t count = profile.size();
    extrusion.positions.reserve(count * (includeCaps ? 6 : 4));
    extrusion.normals.reserve(count * (includeCaps ? 6 : 4));

    // Add normals and positions for the sides.
    for (size_t i = 0; i < count; i++) {
        const Vec2& current = profile[i];
        const Vec2& next = profile[(i + 1) % count];
        addQuad(
            extrusion,
            trimmedStart + side * current.x + up * current.y,
            trimmedStart + side * next.x + up * next.y,
            trimmedEnd + side * next.x + up * next.y,
            trimmedEnd + side * current.x + up * current.y
        );
    }

    // Add normals and positions for the caps.
    if (includeCaps) {
        Vec3 startNormal = tangent * -1.0;
        for (const Vec2& point : profile) {
            extrusion.positions.push_back(trimmedEnd + side * point.x + up * point.y);
            extrusion.normals.push_back(tangent);
        }
        for (const Vec2& point : profile) {
            extrusion.positions.push_back(trimmedStart + side * point.x + up * point.y);
            extrusion.normals.push_back(startNormal);
        }
    }

    DecorationOutput output;
    output.extrusions.push_back(extrusion);
    return output;
}
