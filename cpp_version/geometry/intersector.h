#pragma once
#include "vec2.h"
#include "vec3.h"
#include <vector>
#include <optional>
#include <cmath>

using namespace std;

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

    static optional<Vec2> intersect(const Vec2& s1, const Vec2& e1,
                                       const Vec2& s2, const Vec2& e2);

    static optional<Vec2> intersect(const Vec2& s1, const Vec2& e1,
                                       const Vec2& s2, const Vec2& e2, double thickness);

    static optional<FaceIntersection> intersectFaces(
        const vector<Vec2>& facePositionsA,
        const vector<Vec2>& facePositionsB);

    static vector<IntersectionData> edgeFaceIntersect(
        const Vec3& edge0Start, const Vec3& edge0End, const vector<Vec3>& fPositions, int maxDim);

private:
    Intersector() = delete; // Static class
};

