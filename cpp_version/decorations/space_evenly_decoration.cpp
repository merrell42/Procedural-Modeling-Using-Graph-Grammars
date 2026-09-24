#include "pch.h"
#include "space_evenly_decoration.h"
#include "pending_decoration.h"
#include "../geometry/matrix4.h"
#include <cmath>

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

vector<Instance> SpaceEvenlyDecoration::getInstances(const Vec3& start, const Vec3& end) const {
    if (!child || spacing <= 0.0) {
        return {};
    }
    Vec3 delta = end - start;
    double length = delta.length();
    if (length <= 0.0) {
        return {};
    }
    int count = static_cast<int>(floor(length / spacing + 1e-9));
    if (count < 1) {
        return {};
    }
    double margin = (length - (count - 1) * spacing) / 2.0;
    vector<Instance> instances;
    for (int i = 0; i < count; i++) {
        double distance = margin + i * spacing;
        Vec3 position = start + delta * (distance / length);
        auto childInstances = child->getInstances(Matrix4::translation(
            (float)position.getX(),
            (float)position.getY(),
            (float)position.getZ()
        ));
        instances.insert(instances.end(), childInstances.begin(), childInstances.end());
    }
    return instances;
}
