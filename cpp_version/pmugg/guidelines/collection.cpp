#include "collection.h"

namespace ms {

Collection::Collection(const Stats& stats, const std::string& type) {
    std::map<std::string, std::unique_ptr<AlternativeBase>> properties;
    
    if (globalSettings.getBool("unorderedAlternatives.enabled")) {
        properties[type] = std::make_unique<UnorderedAlternatives>(type);
    } else {
        properties[type] = std::make_unique<AlternativeArray>(type);
    }
    
    node = std::make_unique<Node>(this, stats, "collection", std::move(properties));
}

Node* Collection::getNode() const {
    return node.get();
}

std::vector<Vertex*> Collection::getVertices() const {
    if (node->isDestroyed()) {
        return {};
    }
    return node->get("vertex");
}

std::vector<Face*> Collection::getFaces() const {
    if (node->isDestroyed()) {
        return {};
    }
    return node->get("face");
}

std::vector<RingInstance*> Collection::getRingInstances() const {
    if (node->isDestroyed()) {
        return {};
    }
    return node->get("ringInstance");
}

} // namespace ms 