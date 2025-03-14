#include "intersector.h"
#include <cmath>

namespace ms {


std::optional<Vec2> Intersector::intersect(
    const Vec2& s1, const Vec2& e1,
    const Vec2& s2, const Vec2& e2) {
    return intersect(s1, e1, s2, e2, THICKNESS);
}

std::optional<Vec2> Intersector::intersect(
    const Vec2& s1, const Vec2& e1,
    const Vec2& s2, const Vec2& e2,
    float thickness1
) {
    float thickness2 = thickness1;

    Vec2 r = e1 - s1;
    Vec2 s = e2 - s2;
    float rxs = r.crossZ(s);

    // If the cross product is below the minimum the line segments are almost parallel
    if (std::abs(rxs / r.length() / s.length()) < MIN_CROSS_PRODUCT) {
        return std::nullopt;
    }

    Vec2 q = s2 - s1;
    float qxr = q.crossZ(r);
    float qxs = q.crossZ(s);

    float t = qxs / rxs;
    float u = qxr / rxs;

    // Check if the intersection is within the line segments
    if ((0 <= t && t <= 1) && (0 <= u && u <= 1)) {
        // Get the intersection point
        Vec2 intersection = s1 + r * t;

        // Check if the intersection is within the thickness of both line segments
        Vec2 normal1(-r.y, r.x);
        normal1 = normal1.normalize();
        Vec2 normal2(-s.y, s.x);
        normal2 = normal2.normalize();

        float d1 = std::abs(q.dot(normal1));
        float d2 = std::abs(q.dot(normal2));

        if (d1 <= thickness1 && d2 <= thickness2) {
            return intersection;
        }
    }

    return std::nullopt;
}

std::optional<Intersector::FaceIntersection> Intersector::intersectFaces(
    const std::vector<Vec2>& facePositionsA,
    const std::vector<Vec2>& facePositionsB) {

    // Create copies for manipulation
    std::vector<Vec2> fPositionsA = facePositionsA;
    std::vector<Vec2> fPositionsB = facePositionsB;
    
    // Ensure the faces are oriented correctly
    float areaA = 0;
    float areaB = 0;
    
    size_t Na = fPositionsA.size();
    size_t Nb = fPositionsB.size();
    
    for (size_t i = 0; i < Na; i++) {
        const Vec2& p1 = fPositionsA[i];
        const Vec2& p2 = fPositionsA[(i + 1) % Na];
        areaA += p1.crossZ(p2);
    }
    
    for (size_t i = 0; i < Nb; i++) {
        const Vec2& p1 = fPositionsB[i];
        const Vec2& p2 = fPositionsB[(i + 1) % Nb];
        areaB += p1.crossZ(p2);
    }
    
    // Reverse face positions if needed
    std::vector<Vec2> fPositionsA2 = fPositionsA;
    std::vector<Vec2> fPositionsB2 = fPositionsB;
    
    if (areaA < 0) {
        std::reverse(fPositionsA2.begin(), fPositionsA2.end());
    }
    if (areaB < 0) {
        std::reverse(fPositionsB2.begin(), fPositionsB2.end());
    }

    // Check for intersections between edges
    for (size_t i = 0; i < Na; i++) {
        const Vec2& a0 = fPositionsA2[i];
        const Vec2& a1 = fPositionsA2[(i + 1) % Na];
        
        for (size_t j = 0; j < Nb; j++) {
            const Vec2& b0 = fPositionsB2[j];
            const Vec2& b1 = fPositionsB2[(j + 1) % Nb];
            
            auto intersection = intersect(a0, a1, b0, b1, 0);
            if (intersection) {
                return FaceIntersection{*intersection, 
                                      static_cast<int>(i), 
                                      static_cast<int>(j)};
            }
        }
    }
    
    return std::nullopt;
}

std::vector<IntersectionData> Intersector::lineFaceIntersect(const Vec3& line0Start, const Vec3& line0End, const std::vector<Vec3>& fPositions, int maxDim) {
    size_t N = fPositions.size();
    std::vector<Vec2> fPositions2(N);

    // Drop the specified dimension from fPositions
    for (size_t i = 0; i < N; ++i) {
        fPositions2[i] = fPositions[i].dropDim(maxDim); // Assuming dropDim is a method of Vec3
    }

    Vec2 query0 = line0Start.dropDim(maxDim); // Drop dimension from line0 start
    Vec2 query1 = line0End.dropDim(maxDim); // Drop dimension from line0 end

    std::vector<IntersectionData> intersections;

    for (size_t i = 0; i < N; ++i) {
        Vec2 v0 = fPositions2[i];
        Vec2 v1 = fPositions2[(i + 1) % N]; // Wrap around to the first position

        // Assuming intersect is a method that checks for intersection and returns a position
        auto intersection = intersect(v0, v1, query0, query1);
        if (intersection.has_value()) { // Check if the optional contains a value
            intersections.push_back(IntersectionData(intersection.value(), static_cast<int>(i))); // Use intersection.value() to get the contained value
        }
    }

    return intersections;
}

} // namespace ms 