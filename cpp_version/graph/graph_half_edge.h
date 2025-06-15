#pragma once
#include <memory>
#include "../third_party/json.h"

using Json = nlohmann::json;

class Graph;
class GraphVertex;
class GraphEdge;
class GraphFace;
class Vec3;
struct FaceData;

class GraphHalfEdge {
public:
    explicit GraphHalfEdge(bool forward);
    ~GraphHalfEdge() = default;
    void import(const Json& json);

    bool getForward() const;
    GraphVertex* getVertex() const;
    GraphEdge* getEdge() const;
    GraphHalfEdge* getPrev() const;
    GraphHalfEdge* getNext() const;
    GraphHalfEdge* getTwin() const;
    GraphFace* getFace() const;
    int getVertexIndex() const;
    int getEdgeIndex() const;
    bool isSpliced() const;
    bool isLoopy() const;
    Vec3 getDir() const;

    GraphHalfEdge* connectGraph(Graph* newGraph);
    void connectVertex(GraphVertex* v, int index);
    void disconnect();
    void connectNext(GraphHalfEdge* next);
    void setPrev(GraphHalfEdge* p);
    void setFace(GraphFace* f);

private:
    bool forward;
    GraphVertex* vertex;
    GraphEdge* edge;
    // The indices within the vertex and edge.
    int vertexIndex;
    int edgeIndex;
    GraphHalfEdge* prev;
    GraphHalfEdge* next;
    GraphFace* face;
    Graph* graph;
    int id;

    static int nextId;
};

