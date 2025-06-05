#include "pch.h"
#include <algorithm>
#include "graph.h"
#include "graph_edge.h"
#include "graph_face.h"
#include "graph_half_edge.h"
#include "graph_vertex.h"
#include "../graph_drawing/face.h"
#include "../util/util.h"

namespace ms {

int Graph::nextId = 0;

Graph::Graph() : id(nextId++) {}

void Graph::addVertex(VertexNet* vertex) {
    vertices.push_back(vertex);
}

void Graph::addEdge(GraphEdge* edge) {
    edges.push_back(edge);
}

void Graph::addHalfEdge(GraphHalfEdge* halfEdge) {
    halfEdges.push_back(halfEdge);
}

void Graph::addFace(GraphFace* face) {
    faces.push_back(face);
}

void Graph::removeVertex(VertexNet* vertex) {
    vertices.erase(std::remove(vertices.begin(), vertices.end(), vertex), vertices.end());
}

void Graph::removeEdge(GraphEdge* edge) {
    edges.erase(std::remove(edges.begin(), edges.end(), edge), edges.end());
}

void Graph::removeHalfEdge(GraphHalfEdge* halfEdge) {
    halfEdges.erase(std::remove(halfEdges.begin(), halfEdges.end(), halfEdge), halfEdges.end());
}

void Graph::removeFace(GraphFace* face) {
    faces.erase(std::remove(faces.begin(), faces.end(), face), faces.end());
}

VertexNet* Graph::convertVertex(Graph* graphB, VertexNet* vertexB) {
    return vertexB ? vertices[graphB->vertexIndex(vertexB)] : nullptr;
}

GraphEdge* Graph::convertEdge(Graph* graphB, GraphEdge* edgeB) {
    return edgeB ? edges[graphB->edgeIndex(edgeB)] : nullptr;
}

GraphHalfEdge* Graph::convertHalfEdge(Graph* graphB, GraphHalfEdge* halfEdgeB) {
    return halfEdgeB ? halfEdges[graphB->halfEdgeIndex(halfEdgeB)] : nullptr;
}

GraphFace* Graph::convertFace(Graph* graphB, GraphFace* faceB) {
    return faceB ? faces[graphB->faceIndex(faceB)] : nullptr;
}

int Graph::vertexIndex(VertexNet* vertex) const {
    return vertex ? (int)(std::find(vertices.begin(), vertices.end(), vertex) - vertices.begin()) : -1;
}

int Graph::edgeIndex(GraphEdge* edge) const {
    return edge ? (int)(std::find(edges.begin(), edges.end(), edge) - edges.begin()) : -1;
}

int Graph::halfEdgeIndex(GraphHalfEdge* halfEdge) const {
    return halfEdge ? (int)(std::find(halfEdges.begin(), halfEdges.end(), halfEdge) - halfEdges.begin()) : -1;
}

int Graph::faceIndex(GraphFace* face) const {
    return face ? (int)(std::find(faces.begin(), faces.end(), face) - faces.begin()) : -1;
}

Graph* Graph::import(const Json & json, Shape3D* shape) {
    auto interior = json["interior"];

    auto result = new Graph();

    for (const auto& vertex : interior["vertices"]) {
        (new VertexNet())->connectNet(result);
    }
    for (const auto& edge : interior["edges"]) {
        (new GraphEdge())->connectNet(result);
    }
    for (const auto& halfEdge : interior["halfEdges"]) {
        (new GraphHalfEdge(true))->connectNet(result);
    }
    for (const auto& face : interior["faces"]) {
        (new GraphFace())->connectNet(result);
    }

    for (size_t index = 0; index < interior["vertices"].size(); ++index) {
        result->getVertices()[index]->import(interior["vertices"][index]);
    }
    for (size_t index = 0; index < interior["edges"].size(); ++index) {
        result->getEdges()[index]->import(interior["edges"][index]);
    }
    for (size_t index = 0; index < interior["halfEdges"].size(); ++index) {
        result->getHalfEdges()[index]->import(interior["halfEdges"][index]);
    }
    for (size_t index = 0; index < interior["faces"].size(); ++index) {
        result->getFaces()[index]->import(interior["faces"][index]);
    }

    // Get the types.
    auto edgeVertex = new VertexType();
    for (size_t index = 0; index < json["vertices"].size(); ++index) {
        auto vertexData = json["vertices"][index];
        int type = vertexData["type"].get<int>();
        auto kind = vertexData["kind"].get<std::string>();
        result->getVertices()[index]->kind = kind;
        auto vertexType = (kind == "v") ? shape->vertexTypes[type] : edgeVertex;
        result->getVertices()[index]->setType(vertexType);
    }
    for (size_t index = 0; index < json["edges"].size(); ++index) {
        auto edgeData = json["edges"][index];
        int type = edgeData["type"].get<int>();
        result->getEdges()[index]->setType(shape->edgeTypes[type]);
    }
    for (size_t index = 0; index < json["faces"].size(); ++index) {
        auto faceData = json["faces"][index];
        int type = faceData["type"].get<int>();
        result->getFaces()[index]->setType(shape->faceTypes[type]);
    }
    if (json.contains("morphism")) {
        auto morphism = json["morphism"];
        for (size_t index = 0; index < morphism["halfs"].size(); ++index) {
            int hIndex = morphism["halfs"][index].get<int>();
            auto bHalf = hIndex >= 0 ? result->getHalfEdges()[hIndex] : nullptr;
            result->bHalfEdges.push_back(bHalf);
        }
        for (size_t index = 0; index < morphism["vertices"].size(); ++index) {
            int hIndex = morphism["vertices"][index].get<int>();
            auto bVertex = hIndex >= 0 ? result->getVertices()[hIndex] : nullptr;
            result->bVertices.push_back(bVertex);
        }
        for (size_t index = 0; index < morphism["faces"].size(); ++index) {
            int hIndex = morphism["faces"][index].get<int>();
            auto bFace = hIndex >= 0 ? result->getFaces()[hIndex] : nullptr;
            result->bFaces.push_back(bFace);
        }
    }

    return result;
}

// Remove any spliced edges.
void Graph::removeSplices() {
    // Check if any half edges are spliced
    bool hasSplices = std::any_of(
        getHalfEdges().begin(),
        getHalfEdges().end(),
        [](GraphHalfEdge* half) { return half->isSpliced(); }
    );

    if (!hasSplices) {
        return;
    }

    // This part is dangerous in Javascript.
    // result->getConnectors();

    auto halfEdges = getHalfEdges(); // Make a copy of the vector

    // First pass: merge edges
    for (auto* half : halfEdges) {
        auto* next = half->getNext();
        if (!half->isSpliced() && next && next->isSpliced()) {
            auto* newNext = next->getTwin()->getNext();
            half->getEdge()->merge(newNext->getEdge(), half->getForward());
        }
    }

    // Second pass: remove spliced edges
    halfEdges = getHalfEdges(); // Get fresh copy after merges
    for (auto* half : halfEdges) {
        if (half->isSpliced()) {
            removeHalfEdge(half);
            auto* vertex = half->getVertex();
            // TODO: I'm not sure if this check is necessary.
            if (vertex->inGraph()) {
                removeVertex(vertex);
            }
            auto* edge = half->getEdge();
            // TODO: I'm not sure if this check is necessary.
            if (edge->inGraph()) {
                removeEdge(edge);
            }
        }
    }
}

} // namespace ms 