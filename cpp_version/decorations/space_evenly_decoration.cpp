#include "pch.h"
#include "space_evenly_decoration.h"
#include "pending_decoration.h"
#include "../geometry/matrix4.h"
#include "../util/util.h"

SpaceEvenlyDecoration::SpaceEvenlyDecoration(double spacing)
    : spacing(spacing) {
}

void SpaceEvenlyDecoration::setChild(VertexDecoration* child) {
    this->child = child;
}

void SpaceEvenlyDecoration::resolveChildren(const Decorations& decorations) {
    vector<VertexDecoration*> children = { child };
    resolvePendingChildren<VertexDecoration, const Matrix4&>(children, decorations);
    child = children[0];
}

DecorationOutput SpaceEvenlyDecoration::getOutput(const Vec3& start, const Vec3& end) const {
    if (!child || spacing <= 0.0) {
        return {};
    }
    Vec3 delta = end - start;
    double length = delta.length();
    if (length <= 0.0) {
        return {};
    }
    DecorationOutput output;
    for (double distance : Util::evenlySpaced(0.0, length, spacing)) {
        Vec3 position = start + delta * (distance / length);
        output.append(child->getOutput(Matrix4::translation(position)));
    }
    return output;
}
