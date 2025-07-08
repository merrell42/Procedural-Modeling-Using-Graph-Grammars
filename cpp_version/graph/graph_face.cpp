#include "pch.h"
#include "graph_face.h"
#include "graph_half_edge.h"
#include "graph.h"
#include "../geometry/vec3.h"

GraphFace::GraphFace() 
    : outerComponent(nullptr)
    , type(nullptr)
    , graph(nullptr) {
}

GraphHalfEdge* GraphFace::getOuterComponent() const {
    return outerComponent;
}

Graph* GraphFace::getGraph() const {
    return graph;
}

GraphFace* GraphFace::connectGraph(Graph* graph) {
    this->graph = graph;
    graph->addFace(this);
    return this;
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

// Get the connected half-edges on a face.
// Continue until the path ends or the path loops.
vector<GraphHalfEdge*> GraphFace::getConnectedHalfEdges(const GraphHalfEdge* start) {
    vector<GraphHalfEdge*> result;
    const GraphHalfEdge* current = start;
    do {
        result.push_back(const_cast<GraphHalfEdge*>(current));
        current = current->getNext();
    } while (current && current != start);
    return result;
}

vector<GraphHalfEdge*> GraphFace::getOuterHalfEdges() const {
    if (outerComponent) {
        return getConnectedHalfEdges(outerComponent);
    }
    return vector<GraphHalfEdge*>();
}

// Replace half-edge a with b.
void GraphFace::replaceHalfEdge(GraphHalfEdge* a, GraphHalfEdge* b) {
    if (outerComponent == a) {
        outerComponent = b;
    }
    
    /* for (size_t i = 0; i < innerComponents.size(); i++) {
        if (innerComponents[i] == a) {
            innerComponents[i] = b;
        }
    } */
    b->setFace(this);
}

bool GraphFace::isLoopy() const {
    return outerComponent->isLoopy();
}

void GraphFace::setType(FaceType* newType) {
    type = newType;
}
