#include "pch.h"
#include "graph.h"
#include "graph_edge.h"
#include "graph_face.h"
#include "graph_half_edge.h"
#include "graph_vertex.h"
#include "../util/util.h"

namespace ms {

int HalfEdgeNet::nextId = 0;

HalfEdgeNet::HalfEdgeNet(bool forward)
    : forward(forward)
    , vertex(nullptr)
    , edge(nullptr)
    , vertexIndex(-1)
    , edgeIndex(-1)
    , prev(nullptr)
    , next(nullptr)
    , face(nullptr)
    , network(nullptr)
    , id(nextId++) {}

HalfEdgeNet* HalfEdgeNet::connectNet(Network* net) {
    network = net;
    network->addHalfEdge(this);
    return this;
}

void HalfEdgeNet::connectVertex(VertexNet* v, int index) {
    if (index == -1) {
        // Set to next available spot in the vertex.
        index = (int)v->getHalfEdges().size();
    }
    vertex = v;
    vertexIndex = index;
    vertex->setHalfEdge(this, index);
}

void HalfEdgeNet::disconnectHalfEdge() {
    if (next) {
        next->setPrev(nullptr);
    }
    next = nullptr;
}

void HalfEdgeNet::connectHalfEdge(HalfEdgeNet* n) {
    next = n;
    n->setPrev(this);
}

void HalfEdgeNet::setPrev(HalfEdgeNet* p) {
    prev = p;
}

void HalfEdgeNet::setFace(FaceNet* f) {
    face = f;
}

HalfEdgeNet* HalfEdgeNet::getTwin() const {
    auto halfEdges = edge->getHalfEdges();
    if (halfEdges.size() != 2) {
        throw std::runtime_error("Expected two halfEdges to get a twin.");
    }
    return halfEdges[1 - edgeIndex][0];
}

bool HalfEdgeNet::isSpliced() const {
    return edge && edge->getType()->getSpliced();
}

bool HalfEdgeNet::isLoopy() {
    auto connected = FaceNet::getConnectedHalfEdges(this);
    auto* last = connected.back();
    return last->getNext() != nullptr;
}

const FaceData* HalfEdgeNet::getFaceDatum() const {
    if (!edge) {
        return nullptr;
    }
    return &edge->getType()->getFaceData()[edgeIndex];
}

Vec3 HalfEdgeNet::getDir() const {
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

void HalfEdgeNet::import(const Json& json) {
    forward = json["forward"];
    edgeIndex = json["edgeIndex"];
    vertexIndex = json["vertexIndex"];
    vertex = network->getVertices()[json["vertex"]];
    edge = network->getEdge(json["edge"]);
    prev = network->getHalfEdge(json["prev"]);
    next = network->getHalfEdge(json["next"]);
    face = network->getFaces()[json["face"]];
}

} // namespace ms 