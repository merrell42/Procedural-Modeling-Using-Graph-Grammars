#include "pch.h"
#include "intersector.h"
#include <cmath>

optional<Vec2> Intersector::intersect(
    const Vec2& s1, const Vec2& e1,
    const Vec2& s2, const Vec2& e2) {
    return intersect(s1, e1, s2, e2, THICKNESS);
}

optional<Vec2> Intersector::intersect(
    const Vec2& s1, const Vec2& e1,
    const Vec2& s2, const Vec2& e2,
    // Thickness is currently not used.
    double thickness1
) {
    // double thickness2 = thickness1;

    Vec2 r = e1 - s1;
    Vec2 s = e2 - s2;
    double rxs = r.crossZ(s);

    // If the cross product is below the minimum the line segments are almost parallel
    if (abs(rxs / r.length() / s.length()) < MIN_CROSS_PRODUCT) {
        return nullopt;
    }

    Vec2 q = s2 - s1;
    double qxr = q.crossZ(r);
    double qxs = q.crossZ(s);

    double t = qxs / rxs;
    double u = qxr / rxs;

    // Check if the intersection is within the line segments
    if ((0 < t && t < 1) && (0 < u && u < 1)) {
        // Get the intersection point
        Vec2 intersection = s1 + r * t;
        return intersection;

        // Check if the intersection is within the thickness of both line segments
        /*Vec2 normal1(-r.y, r.x);
        normal1 = normal1.normalize();
        Vec2 normal2(-s.y, s.x);
        normal2 = normal2.normalize();

        double d1 = abs(q.dot(normal1));
        double d2 = abs(q.dot(normal2));

        // This is incorrect. Look at the web version.
        if (d1 <= thickness1 && d2 <= thickness2) {
            return intersection;
        }*/
    }

    return nullopt;
}

vector<IntersectionData> Intersector::edgeFaceIntersect(const Vec3& edge0Start, const Vec3& edge0End, const vector<Vec3>& fPositions, int maxDim) {
    size_t N = fPositions.size();
    vector<Vec2> fPositions2(N);

    // Drop dimension from the positions.
    for (size_t i = 0; i < N; ++i) {
        fPositions2[i] = fPositions[i].dropDim(maxDim);
    }
    Vec2 query0 = edge0Start.dropDim(maxDim);
    Vec2 query1 = edge0End.dropDim(maxDim);

    vector<IntersectionData> intersections;
    for (size_t i = 0; i < N; ++i) {
        Vec2 v0 = fPositions2[i];
        Vec2 v1 = fPositions2[(i + 1) % N];
        auto intersection = intersect(v0, v1, query0, query1);
        if (intersection.has_value()) {
            intersections.push_back(IntersectionData(intersection.value(), static_cast<int>(i)));
        }
    }
    return intersections;
}

