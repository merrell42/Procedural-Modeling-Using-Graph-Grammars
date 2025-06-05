#pragma once
#include <memory>
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

class Graph;
class VertexNet;
class GraphEdge;
class GraphFace;
class Vec3;
struct FaceData;

class GraphHalfEdge {
public:
    explicit GraphHalfEdge(bool forward);
    ~GraphHalfEdge() = default;
    void import(const Json& json);

    // Core accessors
    bool getForward() const { return forward; }
    VertexNet* getVertex() const { return vertex; }
    GraphEdge* getEdge() const { return edge; }
    GraphHalfEdge* getPrev() const { return prev; }
    GraphHalfEdge* getNext() const { return next; }
    GraphHalfEdge* getTwin() const;
    GraphFace* getFace() const { return face; }
    Graph* getGraph() const { return graph; }
    int getVertexIndex() const { return vertexIndex; }
    int getEdgeIndex() const { return edgeIndex; }
    int getId() const { return id; }

    // Graph operations
    GraphHalfEdge* connectNet(Graph* net);
    void connectVertex(VertexNet* v, int index);
    void disconnectHalfEdge();
    void connectHalfEdge(GraphHalfEdge* next);
    void setPrev(GraphHalfEdge* p);
    void setFace(GraphFace* f);

    bool isSpliced() const;
    bool isLoopy();
    const FaceData* getFaceDatum() const;
    Vec3 getDir() const;

private:
    bool forward;
    VertexNet* vertex;
    GraphEdge* edge;
    int vertexIndex;
    int edgeIndex;
    GraphHalfEdge* prev;
    GraphHalfEdge* next;
    GraphFace* face;
    Graph* graph;
    int id;

    static int nextId;
};

} // namespace ms 