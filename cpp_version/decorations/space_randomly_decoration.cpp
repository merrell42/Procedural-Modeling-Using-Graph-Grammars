#include "pch.h"
#include "space_randomly_decoration.h"
#include "pending_decoration.h"
#include "../geometry/matrix4.h"
#include "../util/util.h"

SpaceRandomlyDecoration::SpaceRandomlyDecoration(double spacing)
    : spacing(spacing) {
}

void SpaceRandomlyDecoration::setChild(VertexDecoration* child) {
    this->child = child;
}

void SpaceRandomlyDecoration::resolveChildren(const Decorations& decorations) {
    vector<VertexDecoration*> children = { child };
    resolvePendingChildren<VertexDecoration, const Matrix4&>(children, decorations);
    child = children[0];
}

DecorationOutput SpaceRandomlyDecoration::getOutput(const Vec3& start, const Vec3& end) const {
    if (!child || spacing <= 0.0) {
        return {};
    }
    Vec3 delta = end - start;
    double length = delta.length();
    if (length <= 0.0) {
        return {};
    }
    int count = Util::randomPoisson(length / spacing);
    DecorationOutput output;
    for (int i = 0; i < count; i++) {
        Vec3 position = start + delta * randomValue();
        output.append(child->getOutput(Matrix4::translation(position)));
    }
    return output;
}
