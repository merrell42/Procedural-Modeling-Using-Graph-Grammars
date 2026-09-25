#include "pch.h"
#include "pick_random_decoration.h"
#include "pending_decoration.h"
#include "../util/util.h"

using namespace std;

DECORATION_TEMPLATE
void PickRandomDecoration<Base, Args...>::addChild(Base* child) {
    children.push_back(child);
}

DECORATION_TEMPLATE
void PickRandomDecoration<Base, Args...>::setWeights(const vector<double>& newWeights) {
    weights = newWeights;
}

DECORATION_TEMPLATE
vector<Base*> PickRandomDecoration<Base, Args...>::getChildren() const {
    return children;
}

DECORATION_TEMPLATE
void PickRandomDecoration<Base, Args...>::resolveChildren(const Decorations& decorations) {
    resolvePendingChildren<Base, Args...>(children, decorations);
}

DECORATION_TEMPLATE
DecorationOutput PickRandomDecoration<Base, Args...>::getOutput(Args... args) const {
    int index = -1;
    if (weights.empty()) {
        index = Util::randomInt((int)children.size());
    } else {
        index = Util::randomDistribution(weights);
    }
    if (index < 0 || index >= (int)children.size() || !children[index]) {
        return {};
    }
    return children[index]->getOutput(args...);
}

template class PickRandomDecoration<VertexDecoration, const Matrix4&>;
template class PickRandomDecoration<EdgeDecoration, const Vec3&, const Vec3&>;
template class PickRandomDecoration<FaceDecoration, const Face&>;
