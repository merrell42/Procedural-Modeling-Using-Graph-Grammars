#pragma once
#include "vec2.h"
#include "vec3.h"
#include <vector>
#include <optional>
#include <cmath>

namespace ms {

struct IntersectionData {
    Vec2 pos;
    int index;

    IntersectionData(const Vec2& pos, int idx) : pos(pos), index(idx) {}
};

class Intersector {
public:
    // The minimum angle in degrees between two edge segments for them to intersect.
    // If they are within this angle they are close enough to being parallel that they
    // can be ignored.
    static constexpr double MIN_ANGLE = 1e-6f;
    static constexpr double MIN_CROSS_PRODUCT = 0.000001;
    static constexpr double FAR_DISTANCE = 10000.0;

    // The thickness of the edges in pixels.
    static constexpr double THICKNESS = 0.1f;

    struct IntersectOptions {
        double thickness2;
        IntersectOptions() : thickness2(THICKNESS) {}
    };

    struct FaceIntersection {
        Vec2 pos;
        int indexA;
        int indexB;
    };

    static std::optional<Vec2> intersect(const Vec2& s1, const Vec2& e1,
                                       const Vec2& s2, const Vec2& e2);

    static std::optional<Vec2> intersect(const Vec2& s1, const Vec2& e1,
                                       const Vec2& s2, const Vec2& e2, double thickness);

    static std::optional<FaceIntersection> intersectFaces(
        const std::vector<Vec2>& facePositionsA,
        const std::vector<Vec2>& facePositionsB);

    static std::vector<IntersectionData> edgeFaceIntersect(
        const Vec3& edge0Start, const Vec3& edge0End, const std::vector<Vec3>& fPositions, int maxDim);

private:
    Intersector() = delete; // Static class
};

} // namespace ms 