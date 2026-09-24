#include "pch.h"
#include "union_decoration.h"
#include "pending_decoration.h"

using namespace std;

DECORATION_TEMPLATE
void UnionDecoration<Base, Args...>::addChild(Base* child) {
    children.push_back(child);
}

DECORATION_TEMPLATE
vector<Base*> UnionDecoration<Base, Args...>::getChildren() const {
    return children;
}

DECORATION_TEMPLATE
void UnionDecoration<Base, Args...>::resolveChildren(const Decorations& decorations) {
    resolvePendingChildren<Base, Args...>(children, decorations);
    vector<Base*> resolved;
    for (Base* child : children) {
        resolved.push_back(child);
    }
    children = resolved;
}

DECORATION_TEMPLATE
vector<Instance> UnionDecoration<Base, Args...>::getInstances(Args... args) const {
    vector<Instance> instances;
    for (Base* child : children) {
        auto childInstances = child->getInstances(args...);
        instances.insert(instances.end(), childInstances.begin(), childInstances.end());
    }
    return instances;
}

template class UnionDecoration<VertexDecoration, const Matrix4&>;
template class UnionDecoration<EdgeDecoration, const Vec3&, const Vec3&>;
template class UnionDecoration<FaceDecoration, const Face&>;
