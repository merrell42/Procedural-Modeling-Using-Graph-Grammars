#include "pch.h"
#include <algorithm>
#include "graph.h"
#include "graph_edge.h"
#include "graph_face.h"
#include "graph_half_edge.h"
#include "graph_vertex.h"
#include "../graph_drawing/face.h"
#include "../util/util.h"

int Graph::nextId = 0;

Graph::Graph() : id(nextId++) {
    MemoryCounter::creation("Graph");
}

Graph::~Graph() {
    MemoryCounter::destruction("Graph");
}

void Graph::addVertex(GraphVertex* vertex) {
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

void Graph::removeVertex(GraphVertex* vertex) {
    vertices.erase(remove(vertices.begin(), vertices.end(), vertex), vertices.end());
}

void Graph::removeEdge(GraphEdge* edge) {
    edges.erase(remove(edges.begin(), edges.end(), edge), edges.end());
}

void Graph::removeHalfEdge(GraphHalfEdge* halfEdge) {
    halfEdges.erase(remove(halfEdges.begin(), halfEdges.end(), halfEdge), halfEdges.end());
}

void Graph::removeFace(GraphFace* face) {
    faces.erase(remove(faces.begin(), faces.end(), face), faces.end());
}

Graph* Graph::import(const Json& json, Primitives* shape) {
    auto interior = json["interior"];

    auto result = new Graph();

    for (const auto& vertex : interior["vertices"]) {
        (new GraphVertex())->connectGraph(result);
    }
    for (const auto& edge : interior["edges"]) {
        (new GraphEdge())->connectGraph(result);
    }
    for (const auto& halfEdge : interior["halfEdges"]) {
        (new GraphHalfEdge(true))->connectGraph(result);
    }
    for (const auto& face : interior["faces"]) {
        (new GraphFace())->connectGraph(result);
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
    for (size_t index = 0; index < json["vertexTypes"].size(); ++index) {
        auto vertexData = json["vertexTypes"][index];
        int type = vertexData["type"].get<int>();
        auto kind = vertexData["kind"].get<string>();
        result->getVertices()[index]->kind = kind;
        auto vertexType = (kind == "v") ? shape->vertexTypes[type] : edgeVertex;
        result->getVertices()[index]->setType(vertexType);
    }
    for (size_t index = 0; index < json["edgeTypes"].size(); ++index) {
        auto edgeData = json["edgeTypes"][index];
        int type = edgeData.get<int>();
        result->getEdges()[index]->setType(shape->edgeTypes[type]);
    }
    for (size_t index = 0; index < json["faceTypes"].size(); ++index) {
        auto faceData = json["faceTypes"][index];
        int type = faceData.get<int>();
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

// If the next half edge is spliced, merge pass it. This effectively removes it.
void mergePassSplices(GraphHalfEdge* half) {
    auto* next = half->getNext();
    if (!half->isSpliced() && next && next->isSpliced()) {
        auto* newNext = next->getTwin()->getNext();
        half->getEdge()->merge(newNext->getEdge(), half->getForward());
        // This is necessary when there are multiple slices in a row.
        mergePassSplices(half);
    }
}

// Remove any spliced edges.
void Graph::removeSplices() {
    // Return early if there are no splices.
    auto halfEdges = getHalfEdges();
    bool hasSplices = false;
    for (const auto& half : halfEdges) {
        if (half->isSpliced()) {
            hasSplices = true;
            break;
        }
    }
    if (!hasSplices) {
        return;
    }

    // Merge pass each spliced half-edge.
    for (auto* half : halfEdges) {
        mergePassSplices(half);
    }

    // Remove the spliced half-edges.
    halfEdges = getHalfEdges();
    for (auto* half : halfEdges) {
        if (half->isSpliced()) {
            removeHalfEdge(half);
            removeVertex(half->getVertex());
            removeEdge(half->getEdge());
        }
    }
}

const vector<GraphVertex*>& Graph::getVertices() const {
    return vertices;
}

const vector<GraphEdge*>& Graph::getEdges() const {
    return edges;
}

const vector<GraphHalfEdge*>& Graph::getHalfEdges() const {
    return halfEdges;
}

const vector<GraphFace*>& Graph::getFaces() const {
    return faces;
}

const vector<GraphVertex*>& Graph::getBVertices() const {
    return bVertices;
}

const vector<GraphHalfEdge*>& Graph::getBHalfEdges() const {
    return bHalfEdges;
}

const vector<GraphFace*>& Graph::getBFaces() const {
    return bFaces;
}

GraphEdge* Graph::getEdge(int index) const {
    return index >= 0 ? edges[index] : nullptr;
}

GraphHalfEdge* Graph::getHalfEdge(int index) const {
    return index >= 0 ? halfEdges[index] : nullptr;
}

int Graph::getId() const {
    return id;
}