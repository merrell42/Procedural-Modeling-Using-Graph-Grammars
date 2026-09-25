#pragma once
#include <vector>
#include "edge_decoration.h"
#include "../geometry/vec2.h"

using namespace std;

class ExtrudeDecoration : public EdgeDecoration {
    public:
        ExtrudeDecoration(const vector<Vec2>& polygon, const Vec3& color, double scale = 1.0);
        DecorationOutput getOutput(const Vec3& start, const Vec3& end) const override;

    private:
        vector<Vec2> profile;
        vector<int> triangles;
        bool includeCaps = false;
        Color color;
};
