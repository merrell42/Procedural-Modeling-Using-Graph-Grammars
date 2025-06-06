#pragma once
#include <vector>
#include <memory>
#include "../geometry/vec3.h"
#include "graph_vertex.h"
#include "graph_edge.h"
#include "graph_face.h"
#include "graph_half_edge.h"

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

    const vector<GraphVertex*>& getVertices() const { return vertices; }
    const vector<GraphEdge*>& getEdges() const { return edges; }
    const vector<GraphHalfEdge*>& getHalfEdges() const { return halfEdges; }
    const vector<GraphFace*>& getFaces() const { return faces; }
    const vector<GraphVertex*>& getBVertices() const { return bVertices; }
    const vector<GraphHalfEdge*>& getBHalfEdges() const { return bHalfEdges; }
    const vector<GraphFace*>& getBFaces() const { return bFaces; }
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
    vector<GraphVertex*> vertices;
    vector<GraphEdge*> edges;
    vector<GraphHalfEdge*> halfEdges;
    vector<GraphFace*> faces;

    // Boundary vertices, halfEdges, and faces.
    vector<GraphVertex*> bVertices;
    vector<GraphHalfEdge*> bHalfEdges;
    vector<GraphFace*> bFaces;
    int id;

    static int nextId;
};

