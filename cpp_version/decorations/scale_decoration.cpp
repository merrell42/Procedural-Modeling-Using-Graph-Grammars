#include "pch.h"
#include "scale_decoration.h"
#include "pending_decoration.h"
#include "../util/util.h"

ScaleDecoration::ScaleDecoration(const Vec3& minScale, const Vec3& maxScale, bool uniform)
    : minScale(minScale), maxScale(maxScale), uniform(uniform) {
}

void ScaleDecoration::setChild(VertexDecoration* child) {
    this->child = child;
}

vector<VertexDecoration*> ScaleDecoration::getChildren() const {
    return { child };
}

void ScaleDecoration::resolveChildren(const Decorations& decorations) {
    vector<VertexDecoration*> children = { child };
    resolvePendingChildren<VertexDecoration, const Matrix4&>(children, decorations);
    child = children[0];
}

DecorationOutput ScaleDecoration::getOutput(const Matrix4& transform) const {
    float x;
    float y;
    float z;
    if (uniform) {
        float s = (float)Util::randomUniform(minScale.getX(), maxScale.getX());
        x = s;
        y = s;
        z = s;
    } else {
        x = (float)Util::randomUniform(minScale.getX(), maxScale.getX());
        y = (float)Util::randomUniform(minScale.getY(), maxScale.getY());
        z = (float)Util::randomUniform(minScale.getZ(), maxScale.getZ());
    }
    return child->getOutput(transform * Matrix4::scale(x, y, z));
}
