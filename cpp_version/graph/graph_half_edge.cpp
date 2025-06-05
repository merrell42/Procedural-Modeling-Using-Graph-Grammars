#include "pch.h"
#include "graph.h"
#include "graph_edge.h"
#include "graph_face.h"
#include "graph_half_edge.h"
#include "graph_vertex.h"
#include "../util/util.h"

namespace ms {

int GraphHalfEdge::nextId = 0;

GraphHalfEdge::GraphHalfEdge(bool forward)
    : forward(forward)
    , vertex(nullptr)
    , edge(nullptr)
    , vertexIndex(-1)
    , edgeIndex(-1)
    , prev(nullptr)
    , next(nullptr)
    , face(nullptr)
    , graph(nullptr)
    , id(nextId++) {}

GraphHalfEdge* GraphHalfEdge::connectNet(Graph* net) {
    graph = net;
    graph->addHalfEdge(this);
    return this;
}

void GraphHalfEdge::connectVertex(GraphVertex* v, int index) {
    if (index == -1) {
        // Set to next available spot in the vertex.
        index = (int)v->getHalfEdges().size();
    }
    vertex = v;
    vertexIndex = index;
    vertex->setHalfEdge(this, index);
}

void GraphHalfEdge::disconnectHalfEdge() {
    if (next) {
        next->setPrev(nullptr);
    }
    next = nullptr;
}

void GraphHalfEdge::connectHalfEdge(GraphHalfEdge* n) {
    next = n;
    n->setPrev(this);
}

void GraphHalfEdge::setPrev(GraphHalfEdge* p) {
    prev = p;
}

void GraphHalfEdge::setFace(GraphFace* f) {
    face = f;
}

GraphHalfEdge* GraphHalfEdge::getTwin() const {
    auto halfEdges = edge->getHalfEdges();
    if (halfEdges.size() != 2) {
        throw std::runtime_error("Expected two halfEdges to get a twin.");
    }
    return halfEdges[1 - edgeIndex][0];
}

bool GraphHalfEdge::isSpliced() const {
    return edge && edge->getType()->getSpliced();
}

bool GraphHalfEdge::isLoopy() {
    auto connected = GraphFace::getConnectedHalfEdges(this);
    auto* last = connected.back();
    return last->getNext() != nullptr;
}

const FaceData* GraphHalfEdge::getFaceDatum() const {
    if (!edge) {
        return nullptr;
    }
    return &edge->getType()->getFaceData()[edgeIndex];
}

Vec3 GraphHalfEdge::getDir() const {
    if (!edge) {
        // This maybe should be null.
        return Vec3();
    }
    auto dir = edge->getType()->getDir();
    if (!forward) {
        return dir.scale(-1);
    }
    return dir;
}

void GraphHalfEdge::import(const Json& json) {
    forward = json["forward"];
    edgeIndex = json["edgeIndex"];
    vertexIndex = json["vertexIndex"];
    vertex = graph->getVertices()[json["vertex"]];
    edge = graph->getEdge(json["edge"]);
    prev = graph->getHalfEdge(json["prev"]);
    next = graph->getHalfEdge(json["next"]);
    face = graph->getFaces()[json["face"]];
}

} // namespace ms 