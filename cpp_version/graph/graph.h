#pragma once
#include <vector>
#include <memory>
#include "../geometry/vec3.h"
#include "graph_vertex.h"
#include "graph_edge.h"
#include "graph_face.h"
#include "graph_half_edge.h"

namespace ms {

class GraphVertex;
class GraphEdge;
class GraphHalfEdge;
class GraphFace;
class View;
class Primitives;
struct DrawOptions;

class Graph {
public:
    Graph();
    ~Graph() = default;
    static Graph* import(const Json& json, Primitives* shape = nullptr);

    const std::vector<GraphVertex*>& getVertices() const { return vertices; }
    const std::vector<GraphEdge*>& getEdges() const { return edges; }
    const std::vector<GraphHalfEdge*>& getHalfEdges() const { return halfEdges; }
    const std::vector<GraphFace*>& getFaces() const { return faces; }
    const std::vector<GraphVertex*>& getBVertices() const { return bVertices; }
    const std::vector<GraphHalfEdge*>& getBHalfEdges() const { return bHalfEdges; }
    const std::vector<GraphFace*>& getBFaces() const { return bFaces; }
    GraphEdge* getEdge(int index) const { return index >= 0 ? edges[index] : nullptr; }
    GraphHalfEdge* getHalfEdge(int index) const { return index >= 0 ? halfEdges[index] : nullptr; }
    int getId() const { return id; }

    // Graph operations
    void addVertex(GraphVertex* vertex);
    void addEdge(GraphEdge* edge);
    void addHalfEdge(GraphHalfEdge* halfEdge);
    void addFace(GraphFace* face);

    void removeVertex(GraphVertex* vertex);
    void removeEdge(GraphEdge* edge);
    void removeHalfEdge(GraphHalfEdge* halfEdge);
    void removeFace(GraphFace* face);
    void removeSplices();

    GraphVertex* convertVertex(Graph* graphB, GraphVertex* vertexB);
    GraphEdge* convertEdge(Graph* graphB, GraphEdge* edgeB);
    GraphHalfEdge* convertHalfEdge(Graph* graphB, GraphHalfEdge* halfEdgeB);
    GraphFace* convertFace(Graph* graphB, GraphFace* faceB);

    int vertexIndex(GraphVertex* vertex) const;
    int edgeIndex(GraphEdge* edge) const;
    int halfEdgeIndex(GraphHalfEdge* halfEdge) const;
    int faceIndex(GraphFace* face) const;

private:
    std::vector<GraphVertex*> vertices;
    std::vector<GraphEdge*> edges;
    std::vector<GraphHalfEdge*> halfEdges;
    std::vector<GraphFace*> faces;

    // Boundary vertices, halfEdges, and faces.
    std::vector<GraphVertex*> bVertices;
    std::vector<GraphHalfEdge*> bHalfEdges;
    std::vector<GraphFace*> bFaces;
    int id;

    static int nextId;
};

} // namespace ms 