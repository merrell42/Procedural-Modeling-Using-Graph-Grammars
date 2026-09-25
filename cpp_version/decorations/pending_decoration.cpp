#include "pch.h"
#include "pending_decoration.h"
#include "decorations.h"
#include <iostream>
#include <type_traits>

using namespace std;

namespace {

template <typename Base>
Base* findDecoration(const Decorations& decorations, const string& id) {
    if constexpr (is_same_v<Base, VertexDecoration>) {
        return decorations.getVertexDecoration(id);
    } else if constexpr (is_same_v<Base, EdgeDecoration>) {
        return decorations.getEdgeDecoration(id);
    } else if constexpr (is_same_v<Base, FaceDecoration>) {
        return decorations.getFaceDecoration(id);
    } else {
        static_assert(sizeof(Base) == 0, "Unsupported decoration kind");
        return nullptr;
    }
}

DECORATION_TEMPLATE
Base* resolvePendingChild(Base* child, const Decorations& decorations) {
    auto* pending = dynamic_cast<PendingDecoration<Base, Args...>*>(child);
    if (!pending) {
        return child;
    }
    const string id = pending->getId();
    delete pending;
    Base* resolved = findDecoration<Base>(decorations, id);
    if (!resolved) {
        cerr << "Warning: Unknown decoration child: " << id << endl;
        return nullptr;
    }
    return resolved;
}

} // namespace

DECORATION_TEMPLATE
void resolvePendingChildren(
    vector<Base*>& children,
    const Decorations& decorations
) {
    for (size_t i = 0; i < children.size(); i++) {
        children[i] = resolvePendingChild<Base, Args...>(children[i], decorations);
    }
}

template void resolvePendingChildren<VertexDecoration, const Matrix4&>(
    vector<VertexDecoration*>&,
    const Decorations&
);
template void resolvePendingChildren<EdgeDecoration, const Vec3&, const Vec3&>(
    vector<EdgeDecoration*>&,
    const Decorations&
);
template void resolvePendingChildren<FaceDecoration, const Face&>(
    vector<FaceDecoration*>&,
    const Decorations&
);
