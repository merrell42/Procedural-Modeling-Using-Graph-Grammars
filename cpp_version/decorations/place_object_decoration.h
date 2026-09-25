#pragma once
#include <string>
#include "vertex_decoration.h"

using namespace std;

class PlaceObjectDecoration : public VertexDecoration {
    public:
        explicit PlaceObjectDecoration(const string& object);
        DecorationOutput getOutput(const Matrix4& transform) const override;

    private:
        string object;
};
