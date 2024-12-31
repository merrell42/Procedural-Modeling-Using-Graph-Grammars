#include "bsp_edge.h"
#include "bsp_node.h"

namespace ms {

BspEdge::BspEdge() {
    bspNodes.resize(2, nullptr);
}

BspNode* BspEdge::getChild() const {
    return bspNodes[1];
}

void BspEdge::addNodes(BspNode* parentNode, BspNode* childNode, bool isAbove) {
    // The child must be added first. If the parent comes first, onChange will delete this.
    bspNodes[1] = childNode;
    bspNodes[isAbove ? 1 : 2] = parentNode;
}

void BspEdge::onChanged() {
    if (!getChild()) {
        delete this;
    }
}

} // namespace ms 