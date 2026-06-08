#include "pch.h"
#include "graph.h"
#include "graph_edge.h"
#include "graph_face.h"
#include "graph_half_edge.h"
#include "graph_vertex.h"
#include "../util/util.h"
#include "../util/binary_stream.h"

GraphHalfEdge::GraphHalfEdge(bool forward)
    : forward(forward)
    , vertex(nullptr)
    , edge(nullptr)
    , vertexIndex(-1)
    , edgeIndex(-1)
    , prev(nullptr)
    , next(nullptr)
    , face(nullptr)
    , graph(nullptr) {}

GraphHalfEdge* GraphHalfEdge::connectGraph(Graph* newGraph) {
    graph = newGraph;
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

void GraphHalfEdge::connectEdge(GraphEdge* e, int index) {
    edge = e;
    edgeIndex = index;
    e->addHalfEdge(this, index);
}

void GraphHalfEdge::disconnectEdge() {
    if (edge) {
        edge->removeHalfEdge(this, edgeIndex);
        edge = nullptr;
        edgeIndex = -1;
    }
}

void GraphHalfEdge::disconnect() {
    if (next) {
        next->setPrev(nullptr);
    }
    next = nullptr;
}

void GraphHalfEdge::connectNext(GraphHalfEdge* n) {
    next = n;
    if (n) {
        n->setPrev(this);
    }
}

void GraphHalfEdge::setPrev(GraphHalfEdge* p) {
    prev = p;
}

void GraphHalfEdge::setFace(GraphFace* f) {
    face = f;
}

// The edge pointed in the opposite direction sharing the same edge.
GraphHalfEdge* GraphHalfEdge::getTwin() const {
    auto halfEdges = edge->getHalfEdges();
    if (halfEdges.size() != 2) {
        throw runtime_error("Expected two halfEdges to get a twin.");
    }
    return halfEdges[1 - edgeIndex][0];
}

bool GraphHalfEdge::isSpliced() const {
    return edge && edge->getType()->getSpliced();
}

bool GraphHalfEdge::isLoopy() const {
    auto connected = GraphFace::getConnectedHalfEdges(this);
    auto* last = connected.back();
    return last->getNext() != nullptr;
}

// Get the direction from the edge type. The direction is reverse when forward is false.
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

Json GraphHalfEdge::exportJson(const Graph* graph) const {
    const auto& vertices = graph->getVertices();
    const auto& edges = graph->getEdges();
    const auto& halfEdges = graph->getHalfEdges();
    const auto& faces = graph->getFaces();
    Json json;
    json["forward"] = forward;
    json["edgeIndex"] = edgeIndex;
    json["vertexIndex"] = vertexIndex;
    json["vertex"] = vertex ? indexOf(vertices, vertex) : -1;
    json["edge"] = edge ? indexOf(edges, edge) : -1;
    json["prev"] = prev ? indexOf(halfEdges, prev) : -1;
    json["next"] = next ? indexOf(halfEdges, next) : -1;
    json["face"] = face ? indexOf(faces, face) : -1;
    return json;
}


void GraphHalfEdge::binaryDeserialize(std::istream& in) {
    forward      = bsRead<uint8_t>(in) != 0;
    vertexIndex  = bsRead<int32_t>(in);
    edgeIndex    = bsRead<int32_t>(in);
    int32_t vi   = bsRead<int32_t>(in);
    int32_t ei   = bsRead<int32_t>(in);
    int32_t pi   = bsRead<int32_t>(in);
    int32_t ni   = bsRead<int32_t>(in);
    int32_t fi   = bsRead<int32_t>(in);
    vertex = graph->getVertices()[vi];
    edge   = graph->getEdge(ei);
    prev   = graph->getHalfEdge(pi);
    next   = graph->getHalfEdge(ni);
    face   = graph->getFaces()[fi];
}


bool GraphHalfEdge::getForward() const {
    return forward;
}

GraphVertex* GraphHalfEdge::getVertex() const {
    return vertex;
}

GraphEdge* GraphHalfEdge::getEdge() const {
    return edge;
}

GraphHalfEdge* GraphHalfEdge::getPrev() const {
    return prev;
}

GraphHalfEdge* GraphHalfEdge::getNext() const {
    return next;
}

GraphFace* GraphHalfEdge::getFace() const {
    return face;
}

int GraphHalfEdge::getVertexIndex() const {
    return vertexIndex;
}

int GraphHalfEdge::getEdgeIndex() const {
    return edgeIndex;
}
