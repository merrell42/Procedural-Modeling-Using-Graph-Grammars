#pragma once
#include <memory>
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

class Network;
class VertexNet;
class EdgeNet;
class FaceNet;
class Vec3;
struct FaceData;

class HalfEdgeNet {
public:
    explicit HalfEdgeNet(bool forward);
    ~HalfEdgeNet() = default;

    // Core accessors
    bool getForward() const { return forward; }
    VertexNet* getVertex() const { return vertex; }
    EdgeNet* getEdge() const { return edge; }
    HalfEdgeNet* getPrev() const { return prev; }
    HalfEdgeNet* getNext() const { return next; }
    FaceNet* getFace() const { return face; }
    Network* getNetwork() const { return network; }
    int getVertexIndex() const { return vertexIndex; }
    int getEdgeIndex() const { return edgeIndex; }
    int getId() const { return id; }

    // Network operations
    HalfEdgeNet* connectNet(Network* net);
    void connectVertex(VertexNet* v, int index);
    void disconnectHalfEdge();
    void connectHalfEdge(HalfEdgeNet* next);
    void setPrev(HalfEdgeNet* p);
    void setFace(FaceNet* f);

    // Edge operations
    HalfEdgeNet* getTwin() const;
    // HalfEdgeNet* boundaryToInterior() const;
    bool isSpliced() const;
    bool isLoopy();

    // Face data
    const FaceData* getFaceDatum() const;
    Vec3 getDir() const;

    // Import/Export
    void import(const Json& json);
    // Json export() const;

    // Debug
    void print() const;

private:
    bool forward;
    VertexNet* vertex;
    EdgeNet* edge;
    int vertexIndex;
    int edgeIndex;
    HalfEdgeNet* prev;
    HalfEdgeNet* next;
    FaceNet* face;
    Network* network;
    int id;

    static int nextId;
};

} // namespace ms 