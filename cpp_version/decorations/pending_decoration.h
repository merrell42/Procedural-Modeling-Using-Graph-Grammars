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
class PendingDecoration : public Base {
    public:
        explicit PendingDecoration(const string& id) {
            this->setId(id);
        }
        DecorationOutput getOutput(Args...) const override {
            return {};
        }
};

DECORATION_TEMPLATE
void resolvePendingChildren(
    vector<Base*>& children,
    const Decorations& decorations
);

using VertexPendingDecoration = PendingDecoration<VertexDecoration, const Matrix4&>;
using EdgePendingDecoration = PendingDecoration<EdgeDecoration, const Vec3&, const Vec3&>;
using FacePendingDecoration = PendingDecoration<FaceDecoration, const Face&>;
