#include "pch.h"
#include "rotate_decoration.h"
#include "pending_decoration.h"
#include "../util/util.h"

namespace {
constexpr double degreesToRadians = 3.14159265358979323846 / 180.0;
}

RotateDecoration::RotateDecoration(double minAngle, double maxAngle, const Vec3& axis)
    : minAngle(minAngle), maxAngle(maxAngle), axis(axis) {
}

void RotateDecoration::setChild(VertexDecoration* child) {
    this->child = child;
}

vector<VertexDecoration*> RotateDecoration::getChildren() const {
    return { child };
}

void RotateDecoration::resolveChildren(const Decorations& decorations) {
    vector<VertexDecoration*> children = { child };
    resolvePendingChildren<VertexDecoration, const Matrix4&>(children, decorations);
    child = children[0];
}

vector<Instance> RotateDecoration::getInstances(const Matrix4& transform) const {
    double radians = Util::randomUniform(minAngle, maxAngle) * degreesToRadians;
    Matrix4 rotation = Matrix4::rotation(
        (float)axis.getX(),
        (float)axis.getY(),
        (float)axis.getZ(),
        (float)radians
    );
    return child->getInstances(transform * rotation);
}
