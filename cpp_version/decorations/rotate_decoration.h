#pragma once
#include "vertex_decoration.h"
#include "../geometry/vec3.h"

using namespace std;

class RotateDecoration : public VertexDecoration {
    public:
        RotateDecoration(double minAngle, double maxAngle, const Vec3& axis);
        void setChild(VertexDecoration* child);
        vector<VertexDecoration*> getChildren() const override;
        void resolveChildren(const Decorations& decorations) override;
        vector<Instance> getInstances(const Matrix4& transform) const override;

    private:
        double minAngle;
        double maxAngle;
        Vec3 axis;
        VertexDecoration* child = nullptr;
};
