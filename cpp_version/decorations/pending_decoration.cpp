#include "pch.h"
#include "pending_decoration.h"
#include <iostream>

using namespace std;

namespace {

DECORATION_TEMPLATE
Base* resolvePendingChild(Base* child, const map<string, Base*>& decorations) {
    auto* pending = dynamic_cast<PendingDecoration<Base, Args...>*>(child);
    if (!pending) {
        return child;
    }
    const string id = pending->getId();
    delete pending;
    auto it = decorations.find(id);
    if (it == decorations.end() || !it->second) {
        cerr << "Warning: Unknown decoration child: " << id << endl;
        return nullptr;
    }
    return it->second;
}

} // namespace

DECORATION_TEMPLATE
void resolvePendingChildren(
    vector<Base*>& children,
    const map<string, Base*>& decorations
) {
    for (size_t i = 0; i < children.size(); i++) {
        Base* child = resolvePendingChild<Base, Args...>(children[i], decorations);
        children[i] = child;
        if (child) {
            child->resolveChildren(decorations);
        }
    }
}

template void resolvePendingChildren<VertexDecoration, const Matrix4&>(
    vector<VertexDecoration*>&,
    const map<string, VertexDecoration*>&
);
template void resolvePendingChildren<EdgeDecoration, const Vec3&, const Vec3&>(
    vector<EdgeDecoration*>&,
    const map<string, EdgeDecoration*>&
);
template void resolvePendingChildren<FaceDecoration, const Face&>(
    vector<FaceDecoration*>&,
    const map<string, FaceDecoration*>&
);
