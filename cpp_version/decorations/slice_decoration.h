#pragma once
#include "face_decoration.h"
#include "edge_decoration.h"
#include "vertex_decoration.h"
#include "../geometry/vec3.h"

using namespace std;

class SliceDecoration : public FaceDecoration {
    public:
        SliceDecoration(
            const Vec3& direction,
            double spacing,
            EdgeDecoration* edge,
            VertexDecoration* startVertex,
            VertexDecoration* endVertex
        );
        void resolveChildren(const Decorations& decorations) override;
        DecorationOutput getOutput(const Face& face) const override;

    private:
        Vec3 direction;
        double spacing;
        EdgeDecoration* edge = nullptr;
        VertexDecoration* startVertex = nullptr;
        VertexDecoration* endVertex = nullptr;
};
