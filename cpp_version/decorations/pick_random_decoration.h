#pragma once
#include <string>
#include <vector>
#include "decoration_template.h"
#include "vertex_decoration.h"
#include "edge_decoration.h"
#include "face_decoration.h"

using namespace std;

class Decorations;

DECORATION_TEMPLATE
class PickRandomDecoration : public Base {
    public:
        void addChild(Base* child);
        void setWeights(const vector<double>& newWeights);
        vector<Base*> getChildren() const override;
        void resolveChildren(const Decorations& decorations) override;
        DecorationOutput getOutput(Args... args) const override;

    private:
        vector<Base*> children;
        vector<double> weights;
};

DECORATION_TEMPLATE
using PickRandom = PickRandomDecoration<Base, Args...>;

using VertexPickRandomDecoration = PickRandom<VertexDecoration, const Matrix4&>;
using EdgePickRandomDecoration = PickRandom<EdgeDecoration, const Vec3&, const Vec3&>;
using FacePickRandomDecoration = PickRandom<FaceDecoration, const Face&>;
