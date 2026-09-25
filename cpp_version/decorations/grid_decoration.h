#pragma once
#include "face_decoration.h"
#include "vertex_decoration.h"

using namespace std;

class GridDecoration : public FaceDecoration {
    public:
        explicit GridDecoration(double spacing);
        void setChild(VertexDecoration* child);
        void resolveChildren(const Decorations& decorations) override;
        DecorationOutput getOutput(const Face& face) const override;

    private:
        double spacing;
        VertexDecoration* child = nullptr;
};
