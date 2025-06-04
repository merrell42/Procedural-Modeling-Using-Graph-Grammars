#pragma once
#include <vector>
#include <memory>
#include "../geometry/vec3.h"
#include "graph_vertex.h"
#include "graph_edge.h"
#include "graph_face.h"
#include "graph_half_edge.h"

namespace ms {

class VertexNet;
class EdgeNet;
class HalfEdgeNet;
class FaceNet;
class View;
class Shape3D;
struct DrawOptions;

class Network {
public:
    Network();
    ~Network() = default;
    static Network* import(const Json& json, Shape3D* shape = nullptr);

    const std::vector<VertexNet*>& getVertices() const { return vertices; }
    const std::vector<EdgeNet*>& getEdges() const { return edges; }
    const std::vector<HalfEdgeNet*>& getHalfEdges() const { return halfEdges; }
    const std::vector<FaceNet*>& getFaces() const { return faces; }
    const std::vector<VertexNet*>& getBVertices() const { return bVertices; }
    const std::vector<HalfEdgeNet*>& getBHalfEdges() const { return bHalfEdges; }
    const std::vector<FaceNet*>& getBFaces() const { return bFaces; }
    EdgeNet* getEdge(int index) const { return index >= 0 ? edges[index] : nullptr; }
    HalfEdgeNet* getHalfEdge(int index) const { return index >= 0 ? halfEdges[index] : nullptr; }
    int getId() const { return id; }

    // Network operations
    void addVertex(VertexNet* vertex);
    void addEdge(EdgeNet* edge);
    void addHalfEdge(HalfEdgeNet* halfEdge);
    void addFace(FaceNet* face);

    void removeVertex(VertexNet* vertex);
    void removeEdge(EdgeNet* edge);
    void removeHalfEdge(HalfEdgeNet* halfEdge);
    void removeFace(FaceNet* face);
    void removeSplices();

    VertexNet* convertVertex(Network* networkB, VertexNet* vertexB);
    EdgeNet* convertEdge(Network* networkB, EdgeNet* edgeB);
    HalfEdgeNet* convertHalfEdge(Network* networkB, HalfEdgeNet* halfEdgeB);
    FaceNet* convertFace(Network* networkB, FaceNet* faceB);

    int vertexIndex(VertexNet* vertex) const;
    int edgeIndex(EdgeNet* edge) const;
    int halfEdgeIndex(HalfEdgeNet* halfEdge) const;
    int faceIndex(FaceNet* face) const;

private:
    std::vector<VertexNet*> vertices;
    std::vector<EdgeNet*> edges;
    std::vector<HalfEdgeNet*> halfEdges;
    std::vector<FaceNet*> faces;

    // Boundary vertices, halfEdges, and faces.
    std::vector<VertexNet*> bVertices;
    std::vector<HalfEdgeNet*> bHalfEdges;
    std::vector<FaceNet*> bFaces;
    int id;

    static int nextId;
};

} // namespace ms 