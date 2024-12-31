#pragma once
#include "node.h"
#include "unordered_alternatives.h"
#include "alternative_array.h"
#include <memory>
#include <vector>

namespace ms {

class Collection {
public:
    Collection(const Stats& stats, const std::string& type);
    ~Collection() = default;

    // Core functionality
    Node* getNode() const;
    std::vector<Vertex*> getVertices() const;
    std::vector<Face*> getFaces() const;
    std::vector<RingInstance*> getRingInstances() const;

private:
    std::unique_ptr<Node> node;
};

} // namespace ms 