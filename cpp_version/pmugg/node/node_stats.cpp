#include "node_stats.h"
#include "model.h"
#include <stdexcept>

namespace ms {

const std::vector<std::string> NodeStats::nodeTypes = {
    "cell", "collection", "endpoint", "face", "faceConnection", "faceGroup",
    "line", "lineSegment", "lineState", "vertex", "vertexState",
    "ringInstance", "bspNode", "bspEdge", "bspPolygon"
};

const std::vector<std::string> NodeStats::costTerms = {
    "lineDistance", "valence", "reject"
};

NodeStats::NodeStats()
    : model(nullptr)
    , badCollection(this, "vertex")
    , unloopedCollection(this, "face")
{
    for (const auto& name : nodeTypes) {
        nodes[name] = std::map<int, Node*>();
        count[name] = 0;
    }
    resetCostChange();
    badCollection.getNode()->save();
    unloopedCollection.getNode()->save();
}

NodeStats::~NodeStats() {
    // Clean up any remaining nodes
    for (auto& [_, typeNodes] : nodes) {
        for (auto& [_, node] : typeNodes) {
            delete node;
        }
    }
}

void NodeStats::setModel(Model* m) {
    model = m;
}

Model* NodeStats::getModel() const {
    return model;
}

void NodeStats::addNode(Node* node) {
    auto name = node->getName();
    nodes[name][node->getId()] = node;
    count[name]++;
}

void NodeStats::removeNode(Node* node) {
    auto name = node->getName();
    nodes[name].erase(node->getId());
    count[name]--;
}

void NodeStats::addVertex(Node* node) {
    auto vertex = static_cast<Vertex*>(node->getElement());
    if (vertex->hasConflict()) {
        node->connect(&badCollection);
    } else {
        if (node->has("collection") && node->get<Collection*>("collection")) {
            node->disconnect(&badCollection);
        }
    }
}

void NodeStats::removeVertex(Node* node) {
    if (node->has("collection") && node->get<Collection*>("collection")) {
        node->disconnect(&badCollection);
    }
}

void NodeStats::updateFace(Face* face, bool unlooped) {
    auto node = face->getNode();
    if (unlooped) {
        node->connect(&unloopedCollection);
    } else {
        if (node->has("collection") && node->get<Collection*>("collection")) {
            node->disconnect(&unloopedCollection);
        }
    }
}

std::vector<Vertex*> NodeStats::getBadVertices() const {
    return badCollection.getVertices();
}

std::vector<Face*> NodeStats::getUnloopedFaces() const {
    return unloopedCollection.getFaces();
}

std::vector<void*> NodeStats::getElements(const std::string& name) const {
    if (!nodes.contains(name)) {
        return {};
    }
    std::vector<void*> elements;
    elements.reserve(nodes.at(name).size());
    for (const auto& [_, node] : nodes.at(name)) {
        elements.push_back(node->getElement());
    }
    return elements;
}

int NodeStats::getCount(const std::string& name) const {
    return count.contains(name) ? count.at(name) : 0;
}

void NodeStats::addToChangeList(Node* node) {
    changeList.push_back(node);
}

void NodeStats::apply(std::function<void(Node*)> func) {
    for (const auto& [_, typeNodes] : nodes) {
        for (const auto& [_, node] : typeNodes) {
            func(node);
        }
    }
}

void NodeStats::save() {
    for (auto* node : changeList) {
        node->save();
    }
    changeList.clear();
    resetCostChange();
}

void NodeStats::restore() {
    for (auto* node : changeList) {
        node->restoreDestroyed();
    }
    for (auto* node : changeList) {
        node->restoreCreated();
    }
    changeList.clear();
    resetCostChange();
}

void NodeStats::resetCostChange() {
    for (const auto& term : costTerms) {
        costChange[term] = 0;
    }
    // TODO: Handle detailed cost if needed
}

void NodeStats::verifyAll() {
    for (const auto& [name, typeNodes] : nodes) {
        for (const auto& [_, node] : typeNodes) {
            if (node->isDestroyed()) {
                throw std::runtime_error("Destroyed node found");
            }
        }
    }

    for (const auto& [_, node] : nodes["lineState"]) {
        auto element = static_cast<LineState*>(node->getElement());
        if (!element->cell->activeStates["default"].contains(element)) {
            throw std::runtime_error("Cell missing node");
        }
    }
}

int NodeStats::componentCount() const {
    int count = 0;
    for (const auto& [_, node] : nodes.at("face")) {
        auto face = static_cast<Face*>(node->getElement());
        if (face->isHole()) {
            count++;
        }
    }
    return count;
}

NodeStats* NodeStats::get() {
    return Controller::getCurrentController()->getSynthesizer()->getMutator()->getNodeStats();
}

void NodeStats::verifyAll_() {
    get()->verifyAll();
}

} // namespace ms 