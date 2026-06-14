#pragma once
#include <memory>
#include <iosfwd>
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
    void binaryDeserialize(std::istream& in);
    Json exportJson(const Graph* graph) const;

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
    double getAngle() const;

    GraphHalfEdge* connectGraph(Graph* newGraph);
    void connectVertex(GraphVertex* v, int index);
    void connectEdge(GraphEdge* e, int index);
    void disconnectEdge();
    void disconnect();
    void connectNext(GraphHalfEdge* next);
    void setPrev(GraphHalfEdge* p);
    void setFace(GraphFace* f);

private:
    // True, if the half-edge is pointed in the direction of the edge's type.
    bool forward;

    // The vertex (sometimes called origin) of the half-edge.
    GraphVertex* vertex;
    GraphEdge* edge;

    // The index of this half-edge within the vertex and the edge.
    int vertexIndex;
    int edgeIndex;

    // The previous / next half-edge on the face.
    GraphHalfEdge* prev;
    GraphHalfEdge* next;
    GraphFace* face;
    Graph* graph;
};

