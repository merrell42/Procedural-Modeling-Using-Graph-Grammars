#pragma once
#include <vector>
#include <memory>

namespace ms {

class BspNode;

class BspEdge {
public:
    explicit BspEdge();
    ~BspEdge() = default;

    // Core accessors
    const std::vector<BspNode*>& getBspNodes() const { return bspNodes; }
    BspNode* getChild() const;

    // Node operations
    void addNodes(BspNode* parentNode, BspNode* childNode, bool isAbove);
    void onChanged();

private:
    std::vector<BspNode*> bspNodes;
};

} // namespace ms 