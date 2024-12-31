#pragma once
#include "vec2.h"
#include <vector>
#include <optional>

namespace ms {

class Intersector {
public:
    // The minimum angle in degrees between two line segments for them to intersect.
    // If they are within this angle they are close enough to being parallel that they
    // can be ignored.
    static constexpr float MIN_ANGLE = 1e-6f;
    static constexpr float MIN_CROSS_PRODUCT = 0.0f; // Will be initialized in cpp

    // The thickness of the edges in pixels.
    static constexpr float THICKNESS = 0.1f;

    struct IntersectOptions {
        float thickness2;
        IntersectOptions() : thickness2(THICKNESS) {}
    };

    struct FaceIntersection {
        Vec2 pos;
        int indexA;
        int indexB;
    };

    static std::optional<Vec2> intersect(const Vec2& s1, const Vec2& e1,
                                       const Vec2& s2, const Vec2& e2,
                                       float thickness1,
                                       const IntersectOptions& options = IntersectOptions());

    static std::optional<FaceIntersection> intersectFaces(
        const std::vector<Vec2>& facePositionsA,
        const std::vector<Vec2>& facePositionsB);

private:
    Intersector() = delete; // Static class
};

} // namespace ms 