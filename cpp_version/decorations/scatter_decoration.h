#pragma once
#include "face_decoration.h"
#include "vertex_decoration.h"

using namespace std;

class ScatterDecoration : public FaceDecoration {
    public:
        explicit ScatterDecoration(double density);
        void setChild(VertexDecoration* child);
        void resolveChildren(const Decorations& decorations) override;
        DecorationOutput getOutput(const Face& face) const override;

    private:
        double density;
        VertexDecoration* child = nullptr;
};
