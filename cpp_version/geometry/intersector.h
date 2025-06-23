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
    static constexpr double MIN_CROSS_PRODUCT = 0.000001;
    static constexpr double FAR_DISTANCE = 10000.0;

    // Detect if two line segments intersect.
    static optional<Vec2> intersect(const Vec2& s1, const Vec2& e1, const Vec2& s2, const Vec2& e2);

    // Determine where an edge and a face intersect.
    static vector<IntersectionData> edgeFaceIntersect(
        const Vec3& edgeStart, const Vec3& edgeEnd, const vector<Vec3>& fPositions, int maxDim);

private:
    Intersector() = delete; // Static class
};

