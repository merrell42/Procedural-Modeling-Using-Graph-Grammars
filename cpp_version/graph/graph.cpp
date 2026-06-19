#include "pch.h"
#include <algorithm>
#include "graph.h"
#include "graph_edge.h"
#include "graph_face.h"
#include "graph_half_edge.h"
#include "graph_vertex.h"
#include "../graph_drawing/face.h"
#include "../primitives/primitives.h"
#include "../util/util.h"
#include "../util/binary_stream.h"

std::atomic<int> Graph::nextId{0};

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
        // result->getVertices()[index]->kind = kind;
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

// TODO: We may need to set the type to the outer component type.
/* Graph* Graph::createEmpty(Primitives* shape) {
    auto* result = new Graph();
    if (shape && !shape->faceTypes.empty()) {
        auto* face = (new GraphFace())->connectGraph(result);
        face->setType(shape->faceTypes[0]);
    }
    return result;
} */

Json Graph::exportJson(const Primitives* shape) const {
    Json interior;
    Json verticesJson = Json::array();
    for (auto* vertex : vertices) {
        verticesJson.push_back(vertex->exportJson(halfEdges));
    }
    Json edgesJson = Json::array();
    for (auto* edge : edges) {
        edgesJson.push_back(edge->exportJson(halfEdges));
    }
    Json halfEdgesJson = Json::array();
    for (auto* half : halfEdges) {
        halfEdgesJson.push_back(half->exportJson(this));
    }
    Json facesJson = Json::array();
    for (auto* face : faces) {
        facesJson.push_back(face->exportJson(halfEdges));
    }
    interior["vertices"] = verticesJson;
    interior["edges"] = edgesJson;
    interior["halfEdges"] = halfEdgesJson;
    interior["faces"] = facesJson;

    Json vertexTypesJson = Json::array();
    for (auto* vertex : vertices) {
        Json vertexTypeJson;
        int typeIndex = indexOf(shape->vertexTypes, vertex->getType());
        if (typeIndex >= 0) {
            vertexTypeJson["kind"] = "v";
            vertexTypeJson["type"] = typeIndex;
        } else {
            vertexTypeJson["kind"] = "e";
            int edgeTypeIndex = 0;
            if (auto* edge = vertex->interiorEdge()) {
                edgeTypeIndex = indexOf(shape->edgeTypes, edge->getType());
                if (edgeTypeIndex < 0) {
                    edgeTypeIndex = 0;
                }
            }
            vertexTypeJson["type"] = edgeTypeIndex;
        }
        vertexTypesJson.push_back(std::move(vertexTypeJson));
    }

    Json edgeTypesJson = Json::array();
    for (auto* edge : edges) {
        edgeTypesJson.push_back(indexOf(shape->edgeTypes, edge->getType()));
    }

    Json faceTypesJson = Json::array();
    for (auto* face : faces) {
        faceTypesJson.push_back(indexOf(shape->faceTypes, face->getType()));
    }

    Json morphism;
    Json morphismHalfs = Json::array();
    for (auto* half : bHalfEdges) {
        morphismHalfs.push_back(half ? indexOf(halfEdges, half) : -1);
    }
    Json morphismVertices = Json::array();
    for (auto* vertex : bVertices) {
        morphismVertices.push_back(vertex ? indexOf(vertices, vertex) : -1);
    }
    Json morphismFaces = Json::array();
    for (auto* face : bFaces) {
        morphismFaces.push_back(face ? indexOf(faces, face) : -1);
    }
    morphism["halfs"] = morphismHalfs;
    morphism["vertices"] = morphismVertices;
    morphism["faces"] = morphismFaces;

    Json result;
    result["interior"] = interior;
    result["vertexTypes"] = vertexTypesJson;
    result["edgeTypes"] = edgeTypesJson;
    result["faceTypes"] = faceTypesJson;
    result["morphism"] = morphism;
    return result;
}

Graph* Graph::binaryDeserialize(std::istream& in, Primitives* shape) {
    auto* result = new Graph();

    int32_t vCount  = bsRead<int32_t>(in);
    int32_t eCount  = bsRead<int32_t>(in);
    int32_t heCount = bsRead<int32_t>(in);
    int32_t fCount  = bsRead<int32_t>(in);

    // Allocate all elements first (mirrors Graph::import).
    for (int32_t i = 0; i < vCount;  i++) (new GraphVertex())->connectGraph(result);
    for (int32_t i = 0; i < eCount;  i++) (new GraphEdge())->connectGraph(result);
    for (int32_t i = 0; i < heCount; i++) (new GraphHalfEdge(true))->connectGraph(result);
    for (int32_t i = 0; i < fCount;  i++) (new GraphFace())->connectGraph(result);

    // Vertex fill-in: halfEdge slots + type.
    auto* edgeVertex = new VertexType(); // shared placeholder for "edge kind" vertices
    for (int32_t i = 0; i < vCount; i++) {
        result->vertices[i]->binaryDeserialize(in);
        int32_t typeIdx = bsRead<int32_t>(in);
        result->vertices[i]->setType(typeIdx >= 0 ? shape->vertexTypes[typeIdx] : edgeVertex);
    }

    // Edge fill-in: halfEdge 2D array + type.
    for (int32_t i = 0; i < eCount; i++) {
        result->edges[i]->binaryDeserialize(in);
        result->edges[i]->setType(shape->edgeTypes[bsRead<int32_t>(in)]);
    }

    // HalfEdge fill-in.
    for (int32_t i = 0; i < heCount; i++) {
        result->halfEdges[i]->binaryDeserialize(in);
    }

    // Face fill-in: outerComponent + type.
    for (int32_t i = 0; i < fCount; i++) {
        result->faces[i]->binaryDeserialize(in);
        result->faces[i]->setType(shape->faceTypes[bsRead<int32_t>(in)]);
    }

    // Boundary collections.
    int32_t bvCount = bsRead<int32_t>(in);
    result->bVertices.reserve(bvCount);
    for (int32_t i = 0; i < bvCount; i++) {
        int32_t idx = bsRead<int32_t>(in);
        result->bVertices.push_back(idx >= 0 ? result->vertices[idx] : nullptr);
    }

    int32_t bhCount = bsRead<int32_t>(in);
    result->bHalfEdges.reserve(bhCount);
    for (int32_t i = 0; i < bhCount; i++) {
        int32_t idx = bsRead<int32_t>(in);
        result->bHalfEdges.push_back(idx >= 0 ? result->halfEdges[idx] : nullptr);
    }

    int32_t bfCount = bsRead<int32_t>(in);
    result->bFaces.reserve(bfCount);
    for (int32_t i = 0; i < bfCount; i++) {
        int32_t idx = bsRead<int32_t>(in);
        result->bFaces.push_back(idx >= 0 ? result->faces[idx] : nullptr);
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

template <typename T>
T* remap(const vector<T*>& srcList, const vector<T*>& dstList, T* item) {
    if (!item) {
        return nullptr;
    }
    int idx = indexOf(srcList, item);
    if (idx < 0) {
        throw runtime_error("Graph::copy: element not found");
    }
    return dstList[idx];
}

Graph* Graph::copy() const {
    auto* dst = new Graph();

    vector<GraphVertex*> dstVertices(vertices.size());
    vector<GraphEdge*> dstEdges(edges.size());
    vector<GraphHalfEdge*> dstHalfEdges(halfEdges.size());
    vector<GraphFace*> dstFaces(faces.size());

    for (size_t i = 0; i < vertices.size(); i++) {
        auto* v = (new GraphVertex())->connectGraph(dst);
        v->setType(vertices[i]->getType());
        dstVertices[i] = v;
    }
    for (size_t i = 0; i < edges.size(); i++) {
        auto* e = (new GraphEdge())->connectGraph(dst);
        e->setType(edges[i]->getType());
        dstEdges[i] = e;
    }
    for (size_t i = 0; i < halfEdges.size(); i++) {
        dstHalfEdges[i] = (new GraphHalfEdge(halfEdges[i]->getForward()))->connectGraph(dst);
    }
    for (size_t i = 0; i < faces.size(); i++) {
        auto* f = (new GraphFace())->connectGraph(dst);
        f->setType(faces[i]->getType());
        dstFaces[i] = f;
    }

    for (size_t i = 0; i < vertices.size(); i++) {
        const auto& srcHEs = vertices[i]->getHalfEdges();
        for (size_t j = 0; j < srcHEs.size(); j++) {
            if (srcHEs[j]) {
                dstVertices[i]->setHalfEdge(
                    remap(halfEdges, dstHalfEdges, srcHEs[j]),
                    (int)j
                );
            }
        }
    }

    for (size_t i = 0; i < halfEdges.size(); i++) {
        auto* src = halfEdges[i];
        auto* dstH = dstHalfEdges[i];
        dstH->connectVertex(
            remap(vertices, dstVertices, src->getVertex()),
            src->getVertexIndex()
        );
        if (src->getEdge()) {
            dstH->connectEdge(
                remap(edges, dstEdges, src->getEdge()),
                src->getEdgeIndex()
            );
        }
        dstH->setPrev(remap(halfEdges, dstHalfEdges, src->getPrev()));
        dstH->connectNext(remap(halfEdges, dstHalfEdges, src->getNext()));
        dstH->setFace(remap(faces, dstFaces, src->getFace()));
    }

    for (size_t i = 0; i < faces.size(); i++) {
        dstFaces[i]->setOuterComponent(
            remap(halfEdges, dstHalfEdges, faces[i]->getOuterComponent())
        );
    }

    for (auto* bv : bVertices) {
        dst->bVertices.push_back(remap(vertices, dstVertices, bv));
    }
    for (auto* bh : bHalfEdges) {
        dst->bHalfEdges.push_back(remap(halfEdges, dstHalfEdges, bh));
    }
    for (auto* bf : bFaces) {
        dst->bFaces.push_back(remap(faces, dstFaces, bf));
    }

    return dst;
}

void Graph::setBVertices(const vector<GraphVertex*>& verts) {
    bVertices = verts;
}

void Graph::setBHalfEdges(const vector<GraphHalfEdge*>& halfs) {
    bHalfEdges = halfs;
}

void Graph::merge(Graph* other) {
    if (!other || other == this) {
        return;
    }
    for (auto* v : other->vertices) {
        v->connectGraph(this);
    }
    for (auto* e : other->edges) {
        e->connectGraph(this);
    }
    for (auto* h : other->halfEdges) {
        h->connectGraph(this);
    }
    for (auto* f : other->faces) {
        f->connectGraph(this);
    }
    bVertices.insert(bVertices.end(), other->bVertices.begin(), other->bVertices.end());
    bHalfEdges.insert(bHalfEdges.end(), other->bHalfEdges.begin(), other->bHalfEdges.end());
    bFaces.insert(bFaces.end(), other->bFaces.begin(), other->bFaces.end());
    other->vertices.clear();
    other->edges.clear();
    other->halfEdges.clear();
    other->faces.clear();
    other->bVertices.clear();
    other->bHalfEdges.clear();
    other->bFaces.clear();
}