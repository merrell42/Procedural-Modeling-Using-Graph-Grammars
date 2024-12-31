#include "node.h"
#include "stats.h"
#include <stdexcept>

namespace ms {

int Node::count = 0;

Node::Node(void* element, Stats* stats, const std::string& name,
           const std::map<std::string, Property*>& properties)
    : element(element)
    , stats(stats)
    , name(name)
    , id(count++)
    , properties(properties)
    , destroyed(false)
    , wasDestroyed(true)
    , changed(true)
    , destroyHandler([](){})
    , undestroyHandler([](){})
{
    stats->addToChangeList(this);
    stats->addNode(this);

    for (auto& [_, prop] : properties) {
        prop->setParent(this);
    }
}

Node::~Node() {
    for (auto& [_, prop] : properties) {
        delete prop;
    }
}

std::string Node::getName() const {
    return name;
}

int Node::getId() const {
    return id;
}

Stats* Node::getStats() const {
    return stats;
}

void* Node::getElement() const {
    return element;
}

bool Node::isDestroyed() const {
    return destroyed;
}

bool Node::has(const std::string& name) const {
    return properties.find(name) != properties.end();
}

template<typename T>
T Node::get(const std::string& name) const {
    auto it = properties.find(name);
    if (it == properties.end()) {
        throw std::runtime_error("Property not found: " + name);
    }
    return it->second->get<T>();
}

void Node::add(const std::string& name, void* element, bool atStart) {
    properties.at(name)->add(element, atStart);
}

void Node::remove(const std::string& name, void* element) {
    properties.at(name)->remove(element);
}

void Node::setChangeHandler(const std::string& name, std::function<void()> handler) {
    properties.at(name)->setChangeHandler(handler);
}

void Node::setValue(const std::string& name, const std::any& value) {
    auto prop = properties.at(name);
    if (!dynamic_cast<ValueProperty*>(prop)) {
        throw std::runtime_error("setValue not called on a valueProperty");
    }
    prop->set(value);
    onChanged();
}

void Node::onChanged() {
    if (!changed) {
        changed = true;
        if (stats) {
            stats->addToChangeList(this);
        }
    }
}

void Node::destroy() {
    destroy_();
}

void Node::destroy_() {
    if (destroyed) return;
    
    if (stats) {
        stats->removeNode(this);
    }
    
    destroyed = true;
    
    forEachNeighbor([this](void* neighbor) {
        static_cast<Node*>(neighbor)->onNodeDestroyed(this);
    });
    
    for (auto& [_, prop] : properties) {
        prop->destroy();
    }
    
    destroyHandler();
    onChanged();
}

void Node::save() {
    if (destroyed) return;
    
    changed = false;
    wasDestroyed = destroyed;
    
    for (auto& [_, prop] : properties) {
        prop->save();
    }
}

void Node::restoreDestroyed() {
    changed = false;
    bool undestroyed = false;
    
    if (wasDestroyed) {
        properties.clear();
        return;
    }
    
    if (destroyed) {
        destroyed = false;
        undestroyed = true;
        stats->addNode(this);
    }
    
    for (auto& [_, prop] : properties) {
        prop->restore();
    }
    
    if (undestroyed) {
        undestroyHandler();
    }
}

void Node::restoreCreated() {
    if (wasDestroyed && !destroyed) {
        properties.clear();
        destroy();
    }
}

void Node::setDestroyHandler(std::function<void()> handler) {
    destroyHandler = handler;
}

void Node::setUndestroyHandler(std::function<void()> handler) {
    undestroyHandler = handler;
}

void Node::onNodeDestroyed(Node* node) {
    if (destroyed) return;
    
    auto name = node->getName();
    if (properties.at(name)->onNodeDestroyed(node->getElement())) {
        destroy_();
    }
}

void Node::forEachNeighbor(std::function<void(void*)> func) {
    for (auto& [_, prop] : properties) {
        prop->forEachNeighbor(func);
    }
}

void Node::print() const {
    // Implementation depends on your logging system
}

} // namespace ms 