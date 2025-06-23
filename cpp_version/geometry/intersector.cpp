#include "pch.h"
#include "intersector.h"
#include <cmath>

optional<Vec2> Intersector::intersect(
    const Vec2& s1, const Vec2& e1,
    const Vec2& s2, const Vec2& e2
) {
    Vec2 r = e1 - s1;
    Vec2 s = e2 - s2;
    double rxs = r.crossZ(s);

    // If the cross product is below the minimum the line segments are nearly parallel.
    if (abs(rxs / r.length() / s.length()) < MIN_CROSS_PRODUCT) {
        return nullopt;
    }

    Vec2 q = s2 - s1;
    double qxr = q.crossZ(r);
    double qxs = q.crossZ(s);

    double t = qxs / rxs;
    double u = qxr / rxs;

    // Check if the intersection is within the line segments.
    if ((0 < t && t < 1) && (0 < u && u < 1)) {
        // Get the intersection point.
        Vec2 intersection = s1 + r * t;
        return intersection;
    }
    return nullopt;
}

vector<IntersectionData> Intersector::edgeFaceIntersect(const Vec3& edgeStart, const Vec3& edgeEnd, const vector<Vec3>& fPositions, int maxDim) {
    size_t N = fPositions.size();

    // Drop a dimension from the positions.
    Vec2 eStart = edgeStart.dropDim(maxDim);
    Vec2 eEnd = edgeEnd.dropDim(maxDim);
    vector<Vec2> fPositions2(N);
    for (size_t i = 0; i < N; ++i) {
        fPositions2[i] = fPositions[i].dropDim(maxDim);
    }

    // Intersect each face edge with the original edge.
    vector<IntersectionData> intersections;
    for (size_t i = 0; i < N; ++i) {
        Vec2 fStart = fPositions2[i];
        Vec2 fEnd = fPositions2[(i + 1) % N];
        auto intersection = intersect(fStart, fEnd, eStart, eEnd);
        if (intersection.has_value()) {
            intersections.push_back(IntersectionData(intersection.value(), static_cast<int>(i)));
        }
    }
    return intersections;
}
