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
DecorationOutput UnionDecoration<Base, Args...>::getOutput(Args... args) const {
    DecorationOutput output;
    for (Base* child : children) {
        output.append(child->getOutput(args...));
    }
    return output;
}

template class UnionDecoration<VertexDecoration, const Matrix4&>;
template class UnionDecoration<EdgeDecoration, const Vec3&, const Vec3&>;
template class UnionDecoration<FaceDecoration, const Face&>;
