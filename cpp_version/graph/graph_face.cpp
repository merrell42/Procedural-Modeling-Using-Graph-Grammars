#include "pch.h"
#include "graph_face.h"
#include "graph_half_edge.h"
#include "graph.h"
#include "../geometry/vec3.h"

namespace ms {

GraphFace::GraphFace() 
    : outerComponent(nullptr)
    , type(nullptr)
    , graph(nullptr) {
}

GraphHalfEdge* GraphFace::getOuterComponent() const {
    return outerComponent;
}

const std::vector<GraphHalfEdge*>& GraphFace::getInnerComponents() const {
    return innerComponents;
}

Graph* GraphFace::getGraph() const {
    return graph;
}

GraphFace* GraphFace::connectGraph(Graph* graph) {
    this->graph = graph;
    graph->addFace(this);
    return this;
}

void GraphFace::connectOuter(const std::vector<GraphHalfEdge*>& halfEdges) {
    if (!halfEdges.empty()) {
        outerComponent = halfEdges[0];
        for (auto halfEdge : halfEdges) {
            halfEdge->setFace(this);
        }
    }
}

void GraphFace::makeInner(GraphHalfEdge* halfEdge) {
    innerComponents.push_back(halfEdge);
    outerComponent = nullptr;
}

void GraphFace::copyConnection(const GraphFace* copy) {
    Graph* copyGraph = copy->getGraph();
    outerComponent = graph->convertHalfEdge(copyGraph, copy->getOuterComponent());
    
    innerComponents.clear();
    for (auto halfEdge : copy->getInnerComponents()) {
        innerComponents.push_back(graph->convertHalfEdge(copyGraph, halfEdge));
    }
}

void GraphFace::import(const Json & json) {
    auto halfEdges = graph->getHalfEdges();
    if (halfEdges.size() == 0) {
        return;
    }
    outerComponent = graph->getHalfEdges()[json["outerComponent"]];

    // innerComponents.clear();
    // for (const auto& halfEdge : json["innerComponents"]) {
    //    Vec3 dir = Vec3::import(halfEdge);
    //    // Create a lambda to mimic the getDir behavior
    //    innerComponents.push_back(new GraphHalfEdge(dir));
    // }
}

std::vector<GraphHalfEdge*> GraphFace::getConnectedHalfEdges(GraphHalfEdge* start) {
    std::vector<GraphHalfEdge*> result;
    GraphHalfEdge* current = start;
    do {
        result.push_back(current);
        current = current->getNext();
    } while (current && current != start);
    return result;
}

std::vector<GraphHalfEdge*> GraphFace::getOuterHalfEdges() const {
    if (outerComponent) {
        return getConnectedHalfEdges(outerComponent);
    }
    return std::vector<GraphHalfEdge*>();
}

std::vector<GraphHalfEdge*> GraphFace::getInnerHalfEdges() const {
    std::vector<GraphHalfEdge*> result;
    for (auto component : innerComponents) {
        auto connected = getConnectedHalfEdges(component);
        result.insert(result.end(), connected.begin(), connected.end());
    }
    return result;
}

std::vector<GraphHalfEdge*> GraphFace::getHalfEdges() const {
    auto result = getOuterHalfEdges();
    auto inner = getInnerHalfEdges();
    result.insert(result.end(), inner.begin(), inner.end());
    return result;
}

void GraphFace::replaceHalfEdge(GraphHalfEdge* a, GraphHalfEdge* b, bool force) {
    if ((outerComponent == a) || (force && outerComponent)) {
        outerComponent = b;
    }
    
    for (size_t i = 0; i < innerComponents.size(); i++) {
        if ((innerComponents[i] == a) || (force && innerComponents[i])) {
            innerComponents[i] = b;
        }
    }
    b->setFace(this);
}

bool GraphFace::isLoopy() const {
    return outerComponent->isLoopy();
}

bool GraphFace::inGraph() const {
    auto faces = graph->getFaces();
    return std::find(faces.begin(), faces.end(), this) != faces.end();
}

} // namespace ms 