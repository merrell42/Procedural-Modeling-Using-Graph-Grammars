#pragma once
#include <vector>
#include <memory>
#include <iosfwd>
#include <atomic>
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

// A graph represents an abstract angle graph. Like a graph drawing it
// is a doubly connected edge list (DCEL) with vertices, edges, half-edges, and faces.
// But the vertices and other geometry have no position.
class Graph {
public:
    Graph();
    ~Graph();
    // Import a graph from a JSON object.
    static Graph* import(const Json& json, Primitives* shape = nullptr);
    static Graph* binaryDeserialize(std::istream& in, Primitives* shape);

    const vector<GraphVertex*>& getVertices() const;
    const vector<GraphEdge*>& getEdges() const;
    const vector<GraphHalfEdge*>& getHalfEdges() const;
    const vector<GraphFace*>& getFaces() const;

    const vector<GraphVertex*>& getBVertices() const;
    const vector<GraphHalfEdge*>& getBHalfEdges() const;
    const vector<GraphFace*>& getBFaces() const;

    GraphEdge* getEdge(int index) const;
    GraphHalfEdge* getHalfEdge(int index) const;
    int getId() const;

    void addVertex(GraphVertex* vertex);
    void addEdge(GraphEdge* edge);
    void addHalfEdge(GraphHalfEdge* halfEdge);
    void addFace(GraphFace* face);

    void removeVertex(GraphVertex* vertex);
    void removeEdge(GraphEdge* edge);
    void removeHalfEdge(GraphHalfEdge* halfEdge);
    void removeFace(GraphFace* face);
    void removeSplices();

    Graph* copy() const;
    void merge(Graph* other);
    void setBVertices(const vector<GraphVertex*>& verts);

private:
    vector<GraphVertex*> vertices;
    vector<GraphEdge*> edges;
    vector<GraphHalfEdge*> halfEdges;
    vector<GraphFace*> faces;

    // Boundary vertices, halfEdges, and faces. They are on the graph boundary.
    vector<GraphVertex*> bVertices;
    vector<GraphHalfEdge*> bHalfEdges;
    vector<GraphFace*> bFaces;
    int id;

    static std::atomic<int> nextId;
};

