#pragma once
#include "edge_decoration.h"
#include "vertex_decoration.h"

using namespace std;

class SpaceEvenlyDecoration : public EdgeDecoration {
    public:
        explicit SpaceEvenlyDecoration(double spacing);
        void setChild(VertexDecoration* child);
        void resolveChildren(const Decorations& decorations) override;
        vector<Instance> getInstances(const Vec3& start, const Vec3& end) const override;

    private:
        double spacing;
        VertexDecoration* child = nullptr;
};
