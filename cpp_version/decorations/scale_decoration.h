#pragma once
#include "vertex_decoration.h"
#include "../geometry/vec3.h"

using namespace std;

class ScaleDecoration : public VertexDecoration {
    public:
        ScaleDecoration(const Vec3& minScale, const Vec3& maxScale, bool uniform);
        void setChild(VertexDecoration* child);
        vector<VertexDecoration*> getChildren() const override;
        void resolveChildren(const map<string, VertexDecoration*>& decorations) override;
        vector<Instance> getInstances(const Matrix4& transform) const override;

    private:
        Vec3 minScale;
        Vec3 maxScale;
        bool uniform;
        VertexDecoration* child = nullptr;
};
